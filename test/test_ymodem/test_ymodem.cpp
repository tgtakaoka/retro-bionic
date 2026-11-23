// Host-side tests for the YMODEM session, against lrzsz's sb and rb on a
// pseudo terminal: `pio test -e native`. Skipped where lrzsz is missing.

#include <unity.h>

#include <fcntl.h>
#include <pty.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <string>
#include <vector>

#include "ymodem.h"

using namespace debugger;

namespace {

// Logs the traffic as "<hex" (received) and ">hex" (sent) for a failure.
std::string traffic;

struct PtyLink : YmodemLink {
    int fd = -1;
    // Byte number |damageAt| of the direction |damageIn| picks (received
    // when true) is dropped, once; 0 damages nothing.
    size_t damageAt = 0;
    bool damageIn = true;
    size_t seenIn = 0, seenOut = 0;
    // Held open here too: the slave's last close hangs the tty up, which
    // drops what the child wrote just before it exited.
    int slave = -1;
    int get(uint32_t ms) override {
        auto c = raw(ms);
        if (c >= 0 && damageIn && ++seenIn == damageAt) {
            traffic += "(dropped) ";
            c = raw(ms);
        }
        char hex[8];
        snprintf(hex, sizeof(hex), c < 0 ? "<%d " : "<%02X ", c);
        traffic += hex;
        return c;
    }
    int raw(uint32_t ms) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd, &set);
        timeval tv = {time_t(ms / 1000), suseconds_t(ms % 1000 * 1000)};
        if (select(fd + 1, &set, nullptr, nullptr, &tv) <= 0)
            return -1;
        uint8_t c;
        return read(fd, &c, 1) == 1 ? c : -1;
    }
    void put(const uint8_t *data, size_t len) override {
        if (!damageIn && damageAt > seenOut && damageAt <= seenOut + len) {
            // Split around the dropped byte.
            const auto at = damageAt - seenOut - 1;
            seenOut += len;
            damageAt = 0;
            traffic += "(dropped) ";
            put(data, at);
            put(data + at + 1, len - at - 1);
            return;
        }
        seenOut += len;
        char hex[8];
        for (size_t i = 0; i < len && i < 8; ++i) {
            snprintf(hex, sizeof(hex), ">%02X ", data[i]);
            traffic += hex;
        }
        if (len > 8)
            traffic += ">... ";
        while (len) {
            const auto n = write(fd, data, len);
            if (n <= 0)
                return;
            data += n;
            len -= n;
        }
    }
};

struct MemorySink : YmodemSink {
    std::string name;
    uint32_t size = 0;
    std::vector<uint8_t> data;
    bool refuse = false;
    bool open(const char *n, uint32_t s) override {
        name = n;
        size = s;
        return !refuse;
    }
    bool write(const uint8_t *d, uint16_t len) override {
        data.insert(data.end(), d, d + len);
        return true;
    }
};

struct MemorySource : YmodemSource {
    std::vector<uint8_t> data;
    size_t at = 0;
    uint16_t read(uint8_t *d, uint16_t len) override {
        const auto n = at + len > data.size() ? data.size() - at : len;
        memcpy(d, data.data() + at, n);
        at += n;
        return n;
    }
};

std::vector<uint8_t> pattern(size_t n) {
    std::vector<uint8_t> v(n);
    for (size_t i = 0; i < n; ++i)
        v[i] = uint8_t(i * 7 + (i >> 8));
    return v;
}

const char *DIR = "/tmp/retro-bionic-test/ymodem";
const char *STDERR_FILE = "/tmp/retro-bionic-test/ymodem/stderr.txt";
bool have_lrzsz;

// Runs |argv| in |cwd| with a raw pseudo terminal as its stdin and stdout;
// returns the master side in |link|.
pid_t spawn(const char *const argv[], const char *cwd, PtyLink &link) {
    traffic.clear();
    auto &slave = link.slave;
    termios raw;
    cfmakeraw(&raw);
    TEST_ASSERT_EQUAL(0, openpty(&link.fd, &slave, nullptr, &raw, nullptr));
    const auto pid = fork();
    if (pid == 0) {
        close(link.fd);
        dup2(slave, 0);
        dup2(slave, 1);
        const auto err = open(STDERR_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        dup2(err, 2);
        if (cwd)
            chdir(cwd);
        execvp(argv[0], const_cast<char *const *>(argv));
        _exit(127);
    }
    return pid;
}

// Shows the traffic so far when |result| is not OK.
void expectOk(Ymodem::Result result) {
    if (result != Ymodem::OK) {
        TEST_MESSAGE(Ymodem::name(result));
        TEST_MESSAGE(traffic.size() > 1500 ? traffic.c_str() + traffic.size() - 1500 : traffic.c_str());
    }
    TEST_ASSERT_EQUAL(Ymodem::OK, result);
}

int finish(pid_t pid, PtyLink &link, bool expectFailure = false) {
    int status;
    waitpid(pid, &status, 0);
    close(link.slave);
    close(link.fd);
    const auto code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    if (code != 0 && !expectFailure) {
        // What lrzsz had to say about it.
        const auto f = fopen(STDERR_FILE, "r");
        char text[2000];
        const auto n = f ? fread(text, 1, sizeof(text) - 1, f) : 0;
        text[n] = 0;
        for (auto p = text; *p; ++p)
            if (*p == '\r')
                *p = ' ';
        if (f)
            fclose(f);
        TEST_MESSAGE(text);
        TEST_MESSAGE(traffic.size() > 1500 ? traffic.c_str() + traffic.size() - 1500 : traffic.c_str());
    }
    traffic.clear();
    return code;
}

void writeFile(const char *path, const std::vector<uint8_t> &data) {
    const auto f = fopen(path, "wb");
    fwrite(data.data(), 1, data.size(), f);
    fclose(f);
}

std::vector<uint8_t> readFile(const char *path) {
    std::vector<uint8_t> v;
    const auto f = fopen(path, "rb");
    if (f) {
        uint8_t buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
            v.insert(v.end(), buf, buf + n);
        fclose(f);
    }
    return v;
}

// |damageAt| drops that byte of what sb sends, once.
void receiveFrom_sb(size_t size, size_t damageAt = 0) {
    if (!have_lrzsz)
        TEST_IGNORE_MESSAGE("lrzsz not installed");
    const auto data = pattern(size);
    std::string path = std::string(DIR) + "/sent.bin";
    writeFile(path.c_str(), data);
    PtyLink link;
    link.damageAt = damageAt;
    const char *const argv[] = {"sb", "-q", "-k", path.c_str(), nullptr};
    const auto pid = spawn(argv, nullptr, link);
    Ymodem ymodem(link);
    MemorySink sink;
    uint32_t received;
    expectOk(ymodem.receive(sink, received));
    TEST_ASSERT_EQUAL(0, finish(pid, link));
    TEST_ASSERT_EQUAL_STRING("sent.bin", sink.name.c_str());
    TEST_ASSERT_EQUAL(size, sink.size);
    TEST_ASSERT_EQUAL(size, received);
    TEST_ASSERT_EQUAL(size, sink.data.size());
    if (size)
        TEST_ASSERT_EQUAL_MEMORY(data.data(), sink.data.data(), size);
}

void test_receive_1k_blocks() {
    receiveFrom_sb(3000);
}

void test_receive_exact_blocks() {
    receiveFrom_sb(2048);
}

void test_receive_past_block_255() {
    receiveFrom_sb(300000);
}

void test_receive_short() {
    receiveFrom_sb(100);
}

void test_receive_empty() {
    receiveFrom_sb(0);
}

void test_receive_block_lost_its_first_byte() {
    // The header is 133 bytes and the first data block's STX follows it.
    receiveFrom_sb(3000, 134);
}

void test_receive_block_lost_a_data_byte() {
    receiveFrom_sb(3000, 134 + 600);
}

void test_receive_refused() {
    if (!have_lrzsz)
        TEST_IGNORE_MESSAGE("lrzsz not installed");
    std::string path = std::string(DIR) + "/sent.bin";
    writeFile(path.c_str(), pattern(300));
    PtyLink link;
    const char *const argv[] = {"sb", "-q", "-k", path.c_str(), nullptr};
    const auto pid = spawn(argv, nullptr, link);
    Ymodem ymodem(link);
    MemorySink sink;
    sink.refuse = true;
    uint32_t received;
    TEST_ASSERT_EQUAL(Ymodem::REFUSED, ymodem.receive(sink, received));
    TEST_ASSERT_NOT_EQUAL(0, finish(pid, link, true));
}

// |damageAt| drops that byte of what goes to rb, once.
void sendTo_rb(size_t size, size_t damageAt = 0) {
    if (!have_lrzsz)
        TEST_IGNORE_MESSAGE("lrzsz not installed");
    const auto data = pattern(size);
    std::string path = std::string(DIR) + "/got.bin";
    unlink(path.c_str());
    PtyLink link;
    link.damageAt = damageAt;
    link.damageIn = false;
    const char *const argv[] = {"rb", "-q", "-y", nullptr};
    const auto pid = spawn(argv, DIR, link);
    Ymodem ymodem(link);
    MemorySource source;
    source.data = data;
    expectOk(ymodem.send("got.bin", size, source));
    TEST_ASSERT_EQUAL(0, finish(pid, link));
    const auto got = readFile(path.c_str());
    TEST_ASSERT_EQUAL(size, got.size());
    if (size)
        TEST_ASSERT_EQUAL_MEMORY(data.data(), got.data(), size);
}

void test_send_1k_blocks() {
    sendTo_rb(3000);
}

void test_send_exact_blocks() {
    sendTo_rb(2048);
}

void test_send_past_block_255() {
    sendTo_rb(300000);
}

void test_send_short() {
    sendTo_rb(100);
}

void test_send_empty() {
    sendTo_rb(0);
}

void test_send_block_lost_its_first_byte() {
    // The header is 133 bytes and the first data block's STX follows it.
    sendTo_rb(3000, 134);
}

void test_send_block_lost_a_data_byte() {
    sendTo_rb(3000, 134 + 600);
}

void test_cancelled_from_console() {
    // No other end on the pty, and ^C from the console ends the wait for
    // one (WAIT_MS is a minute, so the timeout itself is not run here).
    PtyLink link;
    int slave;
    TEST_ASSERT_EQUAL(0, openpty(&link.fd, &slave, nullptr, nullptr, nullptr));
    const uint8_t ctrl_c = 0x03;
    write(slave, &ctrl_c, 1);
    Ymodem ymodem(link);
    MemorySource source;
    TEST_ASSERT_EQUAL(Ymodem::CANCELLED, ymodem.send("x", 0, source));
    MemorySink sink;
    uint32_t received;
    write(slave, &ctrl_c, 1);
    TEST_ASSERT_EQUAL(Ymodem::CANCELLED, ymodem.receive(sink, received));
    close(slave);
    close(link.fd);
}

void test_crc16() {
    TEST_ASSERT_EQUAL_HEX16(0x31C3, Ymodem::crc16(0, (const uint8_t *)"123456789", 9));
}

}  // namespace

void setUp() {}
void tearDown() {}

int main() {
    signal(SIGPIPE, SIG_IGN);
    mkdir("/tmp/retro-bionic-test", 0777);
    mkdir(DIR, 0777);
    have_lrzsz = system("command -v sb >/dev/null 2>&1 && command -v rb >/dev/null 2>&1") == 0;
    UNITY_BEGIN();
    RUN_TEST(test_crc16);
    RUN_TEST(test_receive_1k_blocks);
    RUN_TEST(test_receive_exact_blocks);
    RUN_TEST(test_receive_past_block_255);
    RUN_TEST(test_receive_short);
    RUN_TEST(test_receive_empty);
    RUN_TEST(test_receive_block_lost_its_first_byte);
    RUN_TEST(test_receive_block_lost_a_data_byte);
    RUN_TEST(test_receive_refused);
    RUN_TEST(test_send_1k_blocks);
    RUN_TEST(test_send_exact_blocks);
    RUN_TEST(test_send_past_block_255);
    RUN_TEST(test_send_short);
    RUN_TEST(test_send_empty);
    RUN_TEST(test_send_block_lost_its_first_byte);
    RUN_TEST(test_send_block_lost_a_data_byte);
    RUN_TEST(test_cancelled_from_console);
    return UNITY_END();
}
