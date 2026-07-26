#include "../include/mandelbrot_sender.hpp"
#include "../include/sfml_events_handler.hpp"
#include "../include/types_sfml.hpp"
#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include <memory>

struct SharedState {
    bool value = false;
    bool stopped = false;
    bool error = false;
};

struct TestReceiver {
    std::shared_ptr<SharedState> state;

    friend void tag_invoke(ex::set_value_t, TestReceiver &&r) noexcept { r.state->value = true; }
    friend void tag_invoke(ex::set_stopped_t, TestReceiver &&r) noexcept { r.state->stopped = true; }
    friend void tag_invoke(ex::set_error_t, TestReceiver &&r, std::exception_ptr) noexcept { r.state->error = true; }
};

TEST(EventHandler, SendsStoppedWhenWindowShouldClose) {
    RenderSettings settings{100, 100, 100, 2.0};
    SfmlState state(settings);
    state.app_state.should_exit = true;
    auto receiver_state = std::make_shared<SharedState>();

    auto sender = SfmlEventHandler(state.window, state.render_settings, state.app_state);
    auto operation = ex::connect(std::move(sender), TestReceiver{receiver_state});

    ex::start(operation);

    EXPECT_TRUE(receiver_state->stopped);
    EXPECT_FALSE(receiver_state->value);
    EXPECT_FALSE(receiver_state->error);
}

TEST(Pipeline, EventHandlerThenCompute) {
    RenderSettings settings{20, 20, 50, 2.0};
    SfmlState state(settings);
    state.app_state.left_mouse_pressed = true;

    auto pipeline =
        ex::just() |
        ex::let_value([&] { return SfmlEventHandler(state.window, state.render_settings, state.app_state); }) |
        ex::let_value([&] {
            return mandelbrot::MakeComputeSender(&state.fb, state.app_state.need_rerender, state.render_settings,
                                                 state.app_state.viewport);
        });

    auto result = ex::sync_wait(std::move(pipeline));

    ASSERT_TRUE(result);
    EXPECT_TRUE(state.app_state.need_rerender);
    EXPECT_NE(std::count(state.fb.rgba.begin(), state.fb.rgba.end(), 0), state.fb.rgba.size());
}

TEST(ComputeSender, ComputeFb) {
    FrameBuffer fb = FrameBuffer::Make(10, 10);
    std::fill(fb.rgba.begin(), fb.rgba.end(), 0);

    auto sender =
        mandelbrot::MakeComputeSender(&fb, true, RenderSettings{10, 10, 100, 2.0}, AppState::INITIAL_VIEWPORT);

    ex::sync_wait(std::move(sender));

    EXPECT_NE(std::count(fb.rgba.begin(), fb.rgba.end(), 0), fb.rgba.size());
}

TEST(ComputeSender, ReturnSameFb) {
    FrameBuffer fb = FrameBuffer::Make(10, 10);

    auto sender =
        mandelbrot::MakeComputeSender(&fb, false, RenderSettings{10, 10, 100, 2.0}, AppState::INITIAL_VIEWPORT);
    auto result = ex::sync_wait(std::move(sender));

    ASSERT_TRUE(result);
    EXPECT_EQ(std::get<0>(*result), &fb);
}

TEST(ComputeSender, NoModifyBuffer_RerenderIsFalse) {
    FrameBuffer fb = FrameBuffer::Make(10, 10);
    std::fill(fb.rgba.begin(), fb.rgba.end(), 10);

    auto before = fb.rgba;

    auto sender =
        mandelbrot::MakeComputeSender(&fb, false, RenderSettings{10, 10, 100, 2.0}, AppState::INITIAL_VIEWPORT);

    ex::sync_wait(std::move(sender));

    EXPECT_EQ(fb.rgba, before);
}

TEST(EventHandler, ShouldExitStopsPipeline) {
    RenderSettings settings{100, 100, 100, 2.0};
    SfmlState state(settings);
    state.app_state.should_exit = true;

    auto sender = SfmlEventHandler(state.window, state.render_settings, state.app_state);
    auto result = ex::sync_wait(std::move(sender));
    EXPECT_FALSE(result.has_value());
}

TEST(EventHandler, LeftMouseChangesViewport) {
    RenderSettings settings{100, 100, 100, 2.0};
    SfmlState state(settings);

    auto before = state.app_state.viewport;

    state.frame_clock.Reset();
    sf::sleep(sf::milliseconds(200));
    state.app_state.left_mouse_pressed = true;

    auto sender = SfmlEventHandler(state.window, state.render_settings, state.app_state);

    ex::sync_wait(std::move(sender));

    EXPECT_NE(before.width(), state.app_state.viewport.width());
    EXPECT_TRUE(state.app_state.need_rerender);
}

TEST(EventHandler, RightMouseChangesViewport) {
    RenderSettings settings{100, 100, 100, 2.0};
    SfmlState state(settings);

    auto before = state.app_state.viewport;

    state.frame_clock.Reset();
    sf::sleep(sf::milliseconds(200));
    state.app_state.right_mouse_pressed = true;

    auto sender = SfmlEventHandler(state.window, state.render_settings, state.app_state);

    ex::sync_wait(std::move(sender));

    EXPECT_NE(before.width(), state.app_state.viewport.width());
    EXPECT_TRUE(state.app_state.need_rerender);
}
