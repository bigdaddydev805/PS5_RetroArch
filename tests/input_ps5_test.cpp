/* Exercise the real raw joypad callbacks used by the binding screen, and the
 * motion sensors a core reads through the same driver. */
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
#include "../src/input_ps5.cpp"
#include "input_state_wrap.inc"

namespace
{
std::vector<PadSample> pending;
int read_result = 0;
unsigned reads = 0, opens = 0, closes = 0, connects = 0, disconnects = 0;
// The pad's motion-sensor switch: how often it was thrown, the last position,
// and what the service answers.
unsigned motion_calls = 0;
bool motion_last = false;
int32_t motion_result = 0;
std::string last_trace;
PadSample sample()
{
    PadSample p{};
    p.left_x = p.left_y = p.right_x = p.right_y = 128;
    p.connected = 1;
    p.timestamp_us = 10;
    return p;
}
void feed(PadSample p)
{
    pending = {p};
    read_result = 1;
    ps5_joypad.poll();
}
} // namespace
extern "C"
{
    int32_t scePadInit()
    {
        return 0;
    }
    int32_t scePadOpen(int32_t, int32_t, int32_t, const void *)
    {
        ++opens;
        return 1;
    }
    int32_t scePadClose(int32_t)
    {
        ++closes;
        return 0;
    }
    int32_t scePadSetMotionSensorState(int32_t handle, bool enable)
    {
        assert(handle == 1);
        ++motion_calls;
        motion_last = enable;
        return motion_result;
    }
    int32_t scePadRead(int32_t, void *out, int32_t capacity)
    {
        ++reads;
        assert(capacity == 64);
        for (size_t i = 0; i < pending.size(); ++i)
            static_cast<PadSample *>(out)[i] = pending[i];
        pending.clear();
        int result = read_result;
        read_result = 0;
        return result;
    }
    int32_t sceUserServiceInitialize(const void *)
    {
        return 0;
    }
    int32_t sceUserServiceGetInitialUser(int32_t *user)
    {
        *user = 1;
        return 0;
    }
    int32_t sceUserServiceTerminate()
    {
        return 0;
    }
    int32_t sceKernelUsleep(uint32_t)
    {
        return 0;
    }
    void ps5_input_trace(const char *line) noexcept
    {
        last_trace = line;
    }
    bool input_autoconfigure_connect(const char *name, const char *, const char *,
                                     const char *driver, unsigned port, unsigned, unsigned)
    {
        assert(std::strcmp(name, "PS5 Controller") == 0);
        assert(std::strcmp(driver, "ps5") == 0 && port == 0);
        ++connects;
        return true;
    }
    bool input_autoconfigure_disconnect(unsigned port, const char *)
    {
        assert(port == 0);
        ++disconnects;
        return true;
    }
}
// The runloop and video state the script's STOP action arms.
runloop_state_t test_runloop{};
video_driver_state_t test_video{};
runloop_state_t *runloop_state_get_ptr(void)
{
    return &test_runloop;
}
video_driver_state_t *video_state_get_ptr(void)
{
    return &test_video;
}
int main()
{
    void *input = input_ps5.init("");
    assert(ps5_joypad.init(input));
    assert(opens == 1 && !ps5_joypad.query_pad(0));
    auto p = sample();
    feed(p);
    assert(ps5_joypad.query_pad(0) && !ps5_joypad.query_pad(1) && connects == 1);
    assert(ps5_joypad.axis(0, AXIS_POS(0)) == 0);
    size_t n;
    const auto *map = ps5_input_button_map(&n);
    for (size_t i = 0; i < n; ++i)
    {
        p = sample();
        p.buttons = map[i * 2 + 1];
        feed(p);
        for (unsigned b = 0; b < 16; ++b)
            assert(ps5_joypad.button(0, b) == (b == map[i * 2]));
    }
    assert(!ps5_joypad.button(0, NO_BTN) && !ps5_joypad.button(1, 0));
    assert(!ps5_joypad.button(0, HAT_MAP(0, HAT_UP_MASK)));
    p = sample();
    p.left_x = 0;
    p.left_y = 255;
    p.right_x = 255;
    p.right_y = 0;
    p.left_trigger = 255;
    feed(p);
    for (unsigned a = 0; a < 4; ++a)
    {
        int sign = (a == 0 || a == 3) ? -1 : 1;
        assert(ps5_joypad.axis(0, AXIS_NEG(a)) == (sign < 0 ? -32767 : 0));
        assert(ps5_joypad.axis(0, AXIS_POS(a)) == (sign > 0 ? 32767 : 0));
    }
    assert(ps5_joypad.axis(0, AXIS_POS(4)) == 32767);
    assert(ps5_joypad.axis(0, AXIS_NEG(4)) == 0);
    assert(ps5_joypad.button(0, RETRO_DEVICE_ID_JOYPAD_L2));
    assert(!ps5_joypad.button(0, RETRO_DEVICE_ID_JOYPAD_R2));
    assert(ps5_joypad.axis(0, AXIS_NONE) == 0 && ps5_joypad.axis(1, AXIS_POS(0)) == 0);
    assert(ps5_joypad.axis(0, AXIS_POS(31)) == 0);
    // Binding capture polls again in the same frame; no new samples is not a release.
    unsigned before = reads;
    input_ps5.poll(input);
    assert(reads == before);
    ps5_joypad.poll();
    assert(ps5_joypad.axis(0, AXIS_NEG(0)) == -32767);
    assert(ps5_joypad.button(0, RETRO_DEVICE_ID_JOYPAD_L2));

    static retro_keybind_set binds[1]{};
    static retro_keybind autos[RARCH_BIND_LIST_END]{};
    for (unsigned i = 0; i < RARCH_BIND_LIST_END; ++i)
    {
        binds[0][i].valid = true;
        binds[0][i].joykey = NO_BTN;
        binds[0][i].joyaxis = AXIS_NONE;
        autos[i].joykey = NO_BTN;
        autos[i].joyaxis = AXIS_NONE;
    }
    autos[RETRO_DEVICE_ID_JOYPAD_B].joykey = RETRO_DEVICE_ID_JOYPAD_B;
    // Bind B to Square, not Cross, and A to the negative left X axis.
    binds[0][RETRO_DEVICE_ID_JOYPAD_B].joykey = RETRO_DEVICE_ID_JOYPAD_Y;
    binds[0][RETRO_DEVICE_ID_JOYPAD_A].joyaxis = AXIS_NEG(0);
    rarch_joypad_info_t info{};
    info.auto_binds = autos;
    info.joy_idx = 0;
    info.axis_threshold = 0.5f;
    auto mapped = [&](unsigned id)
    {
        return input_state_wrap(&input_ps5, input, &ps5_joypad, nullptr, &info, binds, false, 0,
                                RETRO_DEVICE_JOYPAD, 0, id);
    };
    p = sample();
    p.buttons = pad_button_cross;
    feed(p);
    assert(mapped(RETRO_DEVICE_ID_JOYPAD_B) == 0);
    assert(mapped(RETRO_DEVICE_ID_JOYPAD_MASK) == 0);
    p.buttons = pad_button_square;
    p.left_x = 0;
    feed(p);
    assert(mapped(RETRO_DEVICE_ID_JOYPAD_B) == 1);
    assert(mapped(RETRO_DEVICE_ID_JOYPAD_A) == 1);
    assert(mapped(RETRO_DEVICE_ID_JOYPAD_MASK) ==
           ((1 << RETRO_DEVICE_ID_JOYPAD_B) | (1 << RETRO_DEVICE_ID_JOYPAD_A)));
    // Use the same upstream analog helper as menu navigation, including deadzone.
    const unsigned minus[] = {RARCH_ANALOG_LEFT_X_MINUS, RARCH_ANALOG_LEFT_Y_MINUS,
                              RARCH_ANALOG_RIGHT_X_MINUS, RARCH_ANALOG_RIGHT_Y_MINUS};
    const unsigned plus[] = {RARCH_ANALOG_LEFT_X_PLUS, RARCH_ANALOG_LEFT_Y_PLUS,
                             RARCH_ANALOG_RIGHT_X_PLUS, RARCH_ANALOG_RIGHT_Y_PLUS};
    for (unsigned a = 0; a < 4; ++a)
    {
        autos[minus[a]].joyaxis = AXIS_NEG(a);
        autos[plus[a]].joyaxis = AXIS_POS(a);
    }
    auto menu_axis = [&](unsigned stick, unsigned axis)
    {
        return input_joypad_analog_axis(ANALOG_DPAD_NONE, 0.15f, 1.0f, &ps5_joypad, &info, stick,
                                        axis, binds[0]);
    };
    assert(menu_axis(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X) == -32767);
    assert(menu_axis(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y) == 0);
    p.left_x = 140;
    feed(p);
    assert(menu_axis(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X) == 0);
    p.left_y = 255;
    feed(p);
    assert(menu_axis(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y) > 30000);
    assert(std::strstr(ps5_controller_profile, "input_l_x_minus_axis = \"-0\"\n"));
    assert(std::strstr(ps5_controller_profile, "input_l_y_plus_axis = \"+1\"\n"));
    p.buttons |= pad_button_intercepted;
    feed(p);
    assert(mapped(RETRO_DEVICE_ID_JOYPAD_MASK) == 0 && disconnects == 0);
    p.connected = 0;
    feed(p);
    assert(!ps5_joypad.query_pad(0) && disconnects == 1);
    p = sample();
    feed(p);
    assert(connects == 2);
    // A config reset must refresh automatic binds without a physical reconnect.
    ps5_input_reset_autoconfig();
    ps5_joypad.poll(); // No new samples; the connected sample is retained.
    assert(connects == 3 && opens == 1 && ps5_joypad.query_pad(0));
    ps5_joypad.poll();
    assert(connects == 3); // No per-frame announcement/task storm.
    read_result = -1;
    ps5_joypad.poll();
    assert(!ps5_joypad.query_pad(0) && mapped(RETRO_DEVICE_ID_JOYPAD_MASK) == 0);
    input_bits_t bits;
    std::memset(&bits, 0xff, sizeof(bits));
    ps5_joypad.get_buttons(1, &bits);
    for (auto byte : bits.data)
        assert(byte == 0);
    input_ps5.free(input);
    ps5_joypad.destroy();
    ps5_joypad.destroy();
    assert(closes == 1);
    assert(ps5_joypad.init(input));
    ps5_joypad.destroy();
    assert(opens == 2 && closes == 2);
    ps5_input_reset_autoconfig(); // Safe before/after driver lifetime.

    // The motion sensors: nothing before a core asks; the pad's switch thrown once
    // for both sensors and back once both are off; port 0 only; no light sensor.
    assert(ps5_joypad.init(input));
    feed(sample());
    float value = 1.0f;
    assert(!ps5_joypad.get_sensor_input(0, RETRO_SENSOR_ACCELEROMETER_X, &value));
    assert(!ps5_joypad.get_sensor_input(0, RETRO_SENSOR_GYROSCOPE_Z, &value));
    assert(motion_calls == 0);
    assert(ps5_joypad.set_sensor_state(0, RETRO_SENSOR_ACCELEROMETER_ENABLE, 60));
    assert(motion_calls == 1 && motion_last);
    assert(ps5_joypad.set_sensor_state(0, RETRO_SENSOR_GYROSCOPE_ENABLE, 60));
    assert(motion_calls == 1);
    assert(!ps5_joypad.set_sensor_state(1, RETRO_SENSOR_ACCELEROMETER_ENABLE, 60));
    assert(!ps5_joypad.set_sensor_state(0, RETRO_SENSOR_ILLUMINANCE_ENABLE, 60));
    assert(!ps5_joypad.get_sensor_input(0, RETRO_SENSOR_ILLUMINANCE, &value));
    // The values are the sample's fields in libretro's frame: the accelerometer
    // as read (g, X right, Y up, Z toward the player), the gyroscope (rad/s) as
    // read except for yaw, which the pad reports with the opposite sign.
    p = sample();
    p.acceleration[0] = 0.25f;
    p.acceleration[1] = 1.0f;
    p.acceleration[2] = -0.5f;
    p.angular_velocity[0] = 0.1f;
    p.angular_velocity[1] = 0.2f;
    p.angular_velocity[2] = 0.3f;
    feed(p);
    auto sensor = [&](unsigned id)
    {
        float v = -99.0f;
        assert(ps5_joypad.get_sensor_input(0, id, &v));
        return v;
    };
    assert(sensor(RETRO_SENSOR_ACCELEROMETER_X) == 0.25f);
    assert(sensor(RETRO_SENSOR_ACCELEROMETER_Y) == 1.0f);
    assert(sensor(RETRO_SENSOR_ACCELEROMETER_Z) == -0.5f);
    assert(sensor(RETRO_SENSOR_GYROSCOPE_X) == 0.1f);
    assert(sensor(RETRO_SENSOR_GYROSCOPE_Y) == -0.2f);
    assert(sensor(RETRO_SENSOR_GYROSCOPE_Z) == 0.3f);
    assert(!ps5_joypad.get_sensor_input(1, RETRO_SENSOR_ACCELEROMETER_X, &value));
    // The first sample after switching on was traced, bytes included, and the
    // layout the trace reads is the one the assertions pin.
    assert(last_trace.rfind("input: motion sample bytes 0x08..0x33:", 0) == 0);
    assert(last_trace.find(" 00 00 80 3f ") != std::string::npos); // 1.0f at 0x20 (accel Y)

    // The frame conversion itself, against what was measured on the console
    // (2026-09-30), one physical pose or movement at a time.
    {
        const float still[3] = {0.0f, 0.0f, 0.0f};
        // Left handle down: raw X positive, and libretro X (right side up) positive.
        const float left_handle_down[3] = {0.98f, 0.1f, 0.0f};
        LibretroMotion m = to_libretro_frame(left_handle_down, still);
        assert(m.accel[0] == 0.98f && m.accel[1] == 0.1f && m.accel[2] == 0.0f);
        // Front edge down: raw Z positive, and libretro Z (player's edge up) positive.
        const float front_edge_down[3] = {0.0f, 0.1f, 0.97f};
        m = to_libretro_frame(front_edge_down, still);
        assert(m.accel[2] == 0.97f && m.accel[0] == 0.0f);
        // Resting flat: the 1 g on Y is handed over as 1 g, no unit change.
        const float flat[3] = {0.02f, 1.0f, -0.01f};
        m = to_libretro_frame(flat, still);
        assert(m.accel[1] == 1.0f && m.accel[0] == 0.02f && m.accel[2] == -0.01f);
        // A flat left turn: the pad reports yaw negative; libretro.h wants
        // counter-clockwise seen from above, which a left turn is, positive.
        const float yaw_left[3] = {0.0f, -1.5f, 0.0f};
        m = to_libretro_frame(still, yaw_left);
        assert(m.gyro[1] == 1.5f && m.gyro[0] == 0.0f && m.gyro[2] == 0.0f);
        const float yaw_right[3] = {0.0f, 0.7f, 0.0f};
        assert(to_libretro_frame(still, yaw_right).gyro[1] == -0.7f);
        // The measured bank, left handle up: roll negative is clockwise seen
        // from the player, which is what libretro's Z wants; rad/s unchanged.
        const float bank_left_handle_up[3] = {-0.89f, 0.34f, -3.15f};
        m = to_libretro_frame(still, bank_left_handle_up);
        assert(m.gyro[2] == -3.15f && m.gyro[0] == -0.89f && m.gyro[1] == -0.34f);
        // Pitch passes through: the front edge rising positive.
        const float pitch_up[3] = {1.2f, 0.0f, 0.0f};
        assert(to_libretro_frame(still, pitch_up).gyro[0] == 1.2f);
        // Only yaw is negated: the table says so, and nothing is scaled.
        for (unsigned axis = 0; axis < 3; ++axis)
        {
            assert(accelerometer_sign[axis] == 1.0f);
            assert(gyroscope_sign[axis] == (axis == 1 ? -1.0f : 1.0f));
        }
    }
    // The probe counts the core's reads: six here, reported on the next poll,
    // alongside the values handed over and the requested state.
    actions[0] = ScriptAction{0.0, false, ScriptActionKind::motion, -1};
    action_count = 1;
    last_trace.clear();
    ps5_joypad.poll();
    assert(last_trace.rfind("input: motion sample bytes", 0) == 0);
    action_count = 0;
    for (unsigned id = RETRO_SENSOR_ACCELEROMETER_X; id <= RETRO_SENSOR_GYROSCOPE_Z; ++id)
        (void)sensor(id);
    ps5_joypad.poll();
    trace_motion(*active_pad, 1.0, false);
    assert(last_trace.rfind("input: motion to core: accel +0.2500 +1.0000 -0.5000 gyro +0.1000 "
                            "-0.2000 +0.3000 (core asked accel=1 gyro=1, pad switch on, "
                            "6 core reads last frame, 1 samples)",
                            0) == 0);
    ps5_joypad.poll();
    trace_motion(*active_pad, 1.0, false);
    assert(last_trace.find("0 core reads last frame") != std::string::npos);
    // A frame's batch: the accelerometer is the sharpest sample, whole, not
    // the newest and not a per-axis maximum; the gyroscope is the mean; a
    // sample the shell held is left out of both. Ties go to the newest.
    {
        PadSample first = sample(), peak = sample(), last = sample(), held = sample();
        first.timestamp_us = 20;
        first.acceleration[1] = 1.0f;
        first.angular_velocity[2] = 0.3f;
        peak.timestamp_us = 21;
        peak.acceleration[0] = 2.0f; // 2.06 g: the swing's peak
        peak.acceleration[1] = 0.5f;
        peak.angular_velocity[2] = 0.9f;
        last.timestamp_us = 22;
        last.acceleration[1] = 1.1f;
        last.acceleration[2] = 0.1f; // the newest, no longer at the peak
        last.angular_velocity[2] = 0.6f;
        held.timestamp_us = 19; // the oldest, taken while the shell had the pad
        held.buttons = pad_button_intercepted;
        held.acceleration[0] = 9.0f;
        held.angular_velocity[2] = 9.0f;
        pending = {held, first, peak, last};
        read_result = 4;
        ps5_joypad.poll();
        assert(sensor(RETRO_SENSOR_ACCELEROMETER_X) == 2.0f);
        assert(sensor(RETRO_SENSOR_ACCELEROMETER_Y) == 0.5f);
        assert(sensor(RETRO_SENSOR_ACCELEROMETER_Z) == 0.0f);
        assert(std::fabs(sensor(RETRO_SENSOR_GYROSCOPE_Z) - 0.6f) < 1e-6f);
        assert(sensor(RETRO_SENSOR_GYROSCOPE_Y) == 0.0f);
        trace_motion(*active_pad, 2.0, false);
        assert(last_trace.find("accel +2.0000 +0.5000 +0.0000 gyro +0.0000 -0.0000 +0.6000") !=
               std::string::npos);
        assert(last_trace.find(", 4 samples)") != std::string::npos);
        // At rest every sample is 1 g and the newest wins the tie.
        first.acceleration[1] = last.acceleration[1] = 1.0f;
        last.acceleration[2] = 0.0f;
        last.angular_velocity[2] = first.angular_velocity[2] = 0.0f;
        last.acceleration[0] = 0.25f;
        last.acceleration[1] = std::sqrt(1.0f - 0.25f * 0.25f);
        pending = {first, last};
        read_result = 2;
        ps5_joypad.poll();
        assert(sensor(RETRO_SENSOR_ACCELEROMETER_X) == 0.25f);
        // A batch with nothing usable hands the core nothing.
        pending = {held};
        read_result = 1;
        ps5_joypad.poll();
        assert(sensor(RETRO_SENSOR_ACCELEROMETER_X) == 0.0f);
        pending = {p};
        read_result = 1;
        ps5_joypad.poll();
        assert(sensor(RETRO_SENSOR_ACCELEROMETER_X) == 0.25f);
    }
    // The shell intercepting the pad, or the pad leaving, reads as still: the
    // core is answered, with nothing, rather than sent to another driver.
    p.buttons |= pad_button_intercepted;
    feed(p);
    assert(sensor(RETRO_SENSOR_ACCELEROMETER_Y) == 0.0f);
    p.buttons = 0;
    p.connected = 0;
    feed(p);
    assert(sensor(RETRO_SENSOR_ACCELEROMETER_Y) == 0.0f && motion_calls == 1);
    // A pad that comes back is switched on again, once.
    p.connected = 1;
    feed(p);
    assert(motion_calls == 2 && motion_last);
    assert(sensor(RETRO_SENSOR_ACCELEROMETER_Y) == 1.0f);
    // One sensor off keeps the pad's switch on; the second turns it off.
    assert(ps5_joypad.set_sensor_state(0, RETRO_SENSOR_ACCELEROMETER_DISABLE, 0));
    assert(motion_calls == 2);
    assert(!ps5_joypad.get_sensor_input(0, RETRO_SENSOR_ACCELEROMETER_X, &value));
    assert(sensor(RETRO_SENSOR_GYROSCOPE_Z) == 0.3f);
    assert(ps5_joypad.set_sensor_state(0, RETRO_SENSOR_GYROSCOPE_DISABLE, 0));
    assert(motion_calls == 3 && !motion_last);
    assert(!ps5_joypad.get_sensor_input(0, RETRO_SENSOR_GYROSCOPE_Z, &value));
    // A service that refuses leaves the core with no sensor.
    motion_result = -1;
    assert(!ps5_joypad.set_sensor_state(0, RETRO_SENSOR_GYROSCOPE_ENABLE, 60));
    assert(motion_calls == 4 && !ps5_joypad.get_sensor_input(0, RETRO_SENSOR_GYROSCOPE_X, &value));
    motion_result = 0;
    assert(ps5_joypad.set_sensor_state(0, RETRO_SENSOR_GYROSCOPE_ENABLE, 60));
    assert(motion_calls == 5);
    // Closing the driver with a sensor on switches the pad's off.
    ps5_joypad.destroy();
    assert(motion_calls == 6 && !motion_last && closes == 3);
    // MOTION traces on the poll it arms, and for the seconds it was given.
    assert(ps5_joypad.init(input));
    feed(sample());
    actions[0] = ScriptAction{0.0, false, ScriptActionKind::motion, -1};
    action_count = 1;
    last_trace.clear();
    ps5_joypad.poll();
    assert(last_trace.rfind("input: motion sample bytes", 0) == 0);
    last_trace.clear();
    ps5_joypad.poll();
    assert(last_trace.empty());
    actions[0] = ScriptAction{0.0, false, ScriptActionKind::motion, 60};
    action_count = 1;
    ps5_joypad.poll();
    last_trace.clear();
    ps5_joypad.poll();
    assert(last_trace.rfind("input: motion to core: accel +0.0000 +0.0000 +0.0000 gyro", 0) == 0);
    assert(last_trace.find("(core asked accel=0 gyro=0, pad switch off, 0 core reads last frame, "
                           "1 samples)") != std::string::npos);
    motion_trace_deadline = -1.0;
    action_count = 0;
    ps5_joypad.destroy();

    // STOP ends the run on the next frame, as --max-frames does, and only once.
    test_video.frame_count = 1234;
    actions[0] = ScriptAction{0.0, false, ScriptActionKind::stop};
    action_count = 1;
    run_script_actions();
    assert(test_runloop.max_frames == 1235 && actions[0].done);
    test_video.frame_count = 2000;
    run_script_actions();
    assert(test_runloop.max_frames == 1235);
    action_count = 0;
    std::puts("PS5 joypad: raw binding capture, axes, user mappings, poll retention, lifecycle, "
              "motion sensors and the script's STOP and MOTION PASS");
}
