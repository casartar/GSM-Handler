#include <Arduino.h>
#include <SoftwareSerial.h>

#define DEFAULT_TIMEOUT 1
#define CYCLIC_REQUEST_MS (10 * 1000)

enum class State {
    reset1,
    reset2,
    idle,
    at,
    cmee,
    waitForReady,
    cops,
    csq,
    ati,
    cpms,
    creg,
    cmgf,
    cmgs,

};

enum class Mode {
    init,
    running
};

enum class Result {
    none,
    pending,
    ok,
    error,
};

class Gsm {
public:
    Gsm(uint8_t resetPin);
    void begin(HardwareSerial& serial);
    void handler();
    void sendSMS(String phoneNumber, String smsText);

private:
    void setTimeout(uint16_t timeout_s = DEFAULT_TIMEOUT);
    bool timeoutExpired();
    void sendCommand(String command, uint32_t timeout_s = DEFAULT_TIMEOUT);
    void setState(State state);

    HardwareSerial* HWSerial;
    uint8_t rstPin;

    Result m_result = Result::none;
    State m_state = State::reset1;
    Mode m_mode = Mode::init;

    String expectedCommand;
    uint32_t m_ulTimeout = 0;
    uint32_t m_ulCyclicRequest_ms = 0;
    uint8_t atCounter = 0;
    bool smsReady = false;

    // SMS
    bool m_sendSmsFlag = false;
    String m_smsPhoneNumber;
    String m_smsText;
};