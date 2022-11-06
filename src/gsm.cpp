#include "gsm.h"

SoftwareSerial mySerial(9, 10);

Gsm::Gsm(uint8_t resetPin)
    : rstPin(resetPin)
{
}

void Gsm::begin(HardwareSerial& serial)
{
    HWSerial = &serial;
    pinMode(rstPin, OUTPUT);
    Serial.begin(9600);
    mySerial.begin(9600);
}

void Gsm::handler()
{
    String received = "";
    if (Serial.available()) {
        Serial.setTimeout(1000);
        received = Serial.readStringUntil('\n');
        received.trim();
        mySerial.write("|R:");
        mySerial.write(received.c_str());
        mySerial.write("|E:");
        mySerial.write(expectedCommand.c_str());
        mySerial.write("|");
        if (received == expectedCommand.substring(0, expectedCommand.length() - 1)) {
            mySerial.write("Echo received\r\n");
            if (received.substring(0, 8) == "AT+CMGS=") {
                char buffer[3];
                if (Serial.readBytes(buffer, 2) != 2) {
                    m_result = Result::error;
                    mySerial.write("error\r\n");
                } else {
                    Serial.print(m_smsText + "\r");
                    Serial.write(0x1A);
                }
                mySerial.write(buffer);
            }
        } else if (received == "OK") {
            m_result = Result::ok;
        } else if (received.substring(0, 5) == "ERROR") {
            m_result = Result::error;
        } else if (received.substring(0, 12) == "+CME ERROR: ") {
            m_result = Result::error;
        } else if (received.substring(0, 9) == "SMS Ready") {
            smsReady = true;
        } else if (received.substring(0, 7) == "+COPS: ") {
            /* code */
        } else if (received.substring(0, 7) == "+CPMS: ") {
            /* code */
        } else if (received.substring(0, 7) == "+CREG: ") {
            /* code */
        } else if (received.substring(0, 7) == "+CMGS: ") {
            /* code */
        }
    }

    switch (m_state) {
    case State::reset1:
        switch (m_result) {
        case Result::none:
            m_sendSmsFlag = false;
            smsReady = false;

            digitalWrite(2, LOW);
            setTimeout();
            m_result = Result::pending;
            break;
        default:
            if (timeoutExpired()) {
                setState(State::reset2);
            }
            break;
        }
        break;
    case State::reset2:
        switch (m_result) {
        case Result::none:
            digitalWrite(2, HIGH);
            setTimeout();
            m_result = Result::pending;
            break;
        default:
            if (timeoutExpired()) {
                setState(State::at);
            }
            break;
        }
        break;
    case State::idle:
        break;
    case State::at:
        switch (m_result) {
        case Result::none:
            sendCommand("AT\r");
            break;
        case Result::ok:
            atCounter = 0;
            setState(State::cmee);
            break;
        case Result::error:
            setState(State::reset1);
            break;
        case Result::pending:
            if (timeoutExpired()) {
                atCounter++;
                setState(atCounter > 8 ? State::reset1 : m_state = State::at);
            }
            break;
        }
        break;
    case State::cmee:
        switch (m_result) {
        case Result::none:
            sendCommand("AT+CMEE=2\r");
            break;
        case Result::ok:
            setState(State::waitForReady);
            break;
        case Result::error:
            setState(State::reset1);
            break;
        case Result::pending:
            if (timeoutExpired()) {
                mySerial.write("CMEE-Timeout\r\n");
                setState(State::reset1);
            }
            break;
        }
        break;
    case State::waitForReady:
        switch (m_result) {
        case Result::none:
            setTimeout(15);
            m_result = Result::pending;
            break;
        default:
            if (timeoutExpired() || smsReady == true) {
                setState(State::cops);
            }
            break;
        }
        break;
    case State::cops:
        switch (m_result) {
        case Result::none:
            sendCommand("AT+COPS?\r");
            break;
        case Result::ok:
            setState(m_mode == Mode::init ? State::csq : State::creg);
            break;
        case Result::error:
            setState(State::reset1);
            break;
        case Result::pending:
            if (timeoutExpired()) {
                mySerial.write("COPS-Timeout\r\n");
                setState(State::reset1);
            }
            break;
        }
        break;
    case State::csq:
        switch (m_result) {
        case Result::none:
            sendCommand("AT+CSQ\r");
            break;
        case Result::ok:
            setState(m_mode == Mode::init ? State::ati : State::idle);
            break;
        case Result::error:
            setState(State::reset1);
            break;
        case Result::pending:
            if (timeoutExpired()) {
                mySerial.write("COPS-Timeout\r\n");
                setState(State::reset1);
            }
            break;
        }
        break;
    case State::ati:
        switch (m_result) {
        case Result::none:
            sendCommand("ATI\r");
            break;
        case Result::ok:
            setState(State::cpms);
            break;
        case Result::error:
            setState(State::reset1);
            break;
        case Result::pending:
            if (timeoutExpired()) {
                mySerial.write("ATI-Timeout\r\n");
                setState(State::reset1);
            }
            break;
        }
        break;
    case State::cpms:
        switch (m_result) {
        case Result::none:
            sendCommand("AT+CPMS=\"SM\",\"SM\",\"SM\"\r", 5);
            break;
        case Result::ok:
            setState(State::creg);
            break;
        case Result::error:
            setState(State::reset1);
            break;
        case Result::pending:
            if (timeoutExpired()) {
                mySerial.write("CPMS-Timeout\r\n");
                setState(State::reset1);
            }
            break;
        }
        break;
    case State::creg:
        switch (m_result) {
        case Result::none:
            sendCommand("AT+CREG?\r");
            break;
        case Result::ok:
            setState(m_mode == Mode::init ? State::cmgf : State::csq);
            break;
        case Result::error:
            setState(State::reset1);
            break;
        case Result::pending:
            if (timeoutExpired()) {
                mySerial.write("CREG-Timeout\r\n");
                setState(State::reset1);
            }
            break;
        }
        break;
    case State::cmgf:
        switch (m_result) {
        case Result::none:
            sendCommand("AT+CMGF=1\r");
            break;
        case Result::ok:
            setState(State::idle);
            m_mode = Mode::running;
            break;
        case Result::error:
            setState(State::reset1);
            break;
        case Result::pending:
            if (timeoutExpired()) {
                mySerial.write("CMGF-Timeout\r\n");
                setState(State::reset1);
            }
            break;
        }
        break;
    case State::cmgs:
        switch (m_result) {
        case Result::none:
            sendCommand("AT+CMGS=\"" + m_smsPhoneNumber + "\"\r", 60);
            break;
        case Result::ok:
            m_sendSmsFlag = false;
            setState(State::idle);
            break;
        case Result::error:
            m_sendSmsFlag = false;
            setState(State::reset1);
            break;
        case Result::pending:
            if (timeoutExpired()) {
                m_sendSmsFlag = false;
                mySerial.write("CMGS-Timeout\r\n");
                setState(State::reset1);
            }
            break;
        }
        break;
    default:
        break;
    }

    switch (m_mode) {
    case Mode::init:
        /* code */
        break;
    case Mode::running:
        if (m_state == State::idle) {
            if (millis() > m_ulCyclicRequest_ms) {
                m_ulCyclicRequest_ms = millis() + CYCLIC_REQUEST_MS;
                setState(State::cops);
            } else if (m_sendSmsFlag) {
                setState(State::cmgs);
            }
        }
        break;
    }
}

void Gsm::setTimeout(uint16_t timeout_s)
{
    m_ulTimeout = millis() + timeout_s * 1000;
}
bool Gsm::timeoutExpired()
{
    return millis() > m_ulTimeout;
}
void Gsm::sendCommand(String command, uint32_t timeout_s)
{
    expectedCommand = command;
    Serial.write(command.c_str());
    setTimeout(timeout_s);
    m_result = Result::pending;
}
void Gsm::sendSMS(String phoneNumber, String smsText)
{
    mySerial.write("sendSMS");
    m_sendSmsFlag = true;
    m_smsPhoneNumber = phoneNumber;
    m_smsText = smsText;
}
void Gsm::setState(State state)
{
    m_state = state;
    m_result = Result::none;
}