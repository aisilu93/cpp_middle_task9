#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window/Mouse.hpp>
#include <stdexec/execution.hpp>

#include "types_core.hpp"

class SfmlEventHandler {
public:
    using sender_concept = stdexec::sender_t;

    template <typename Receiver>
    struct OperationState {
        Receiver receiver_;
        sf::RenderWindow &window_;
        RenderSettings render_settings_;
        AppState &state_;

        static constexpr float ZOOM_INTERVAL_MS = 100.0f;

        template <typename R>
        explicit OperationState(R &&r, sf::RenderWindow &window, RenderSettings render_settings, AppState &state)
            : receiver_{std::forward<R>(r)}, window_{window}, render_settings_{render_settings}, state_{state} {}

        /* Ваш код метода start() здесь */
        void start() noexcept {
            try {
                HandleEvents();
                if (state_.should_exit) {
                    ex::set_stopped(std::move(receiver_));
                    return;
                }
                if (state_.auto_zoom_enabled)
                    HandleAutoZoom();
                else if (state_.left_mouse_pressed) {
                    auto pos = sf::Mouse::getPosition(window_);
                    ZoomToPoint(pos.x, pos.y, true);
                } else if (state_.right_mouse_pressed) {
                    auto pos = sf::Mouse::getPosition(window_);
                    ZoomToPoint(render_settings_.width / 2, render_settings_.height / 2, false);
                }

                ex::set_value(std::move(receiver_));
            } catch (...) {
                ex::set_error(std::move(receiver_), std::current_exception());
            }
        }

    private:
        void HandleEvents() {
            sf::Event event;
            while (window_.pollEvent(event)) {
                switch (event.type) {
                case sf::Event::Closed:
                    state_.should_exit = true;
                    break;
                /* Ваш код здесь  */
                case sf::Event::MouseButtonPressed:
                    HandleMousePress(event.mouseButton);
                    break;
                case sf::Event::MouseButtonReleased:
                    HandleMouseRelease(event.mouseButton);
                    break;
                case sf::Event::KeyPressed:
                    HandleKeyPress(event.key);
                    break;
                default:
                    break;
                }
            }
        }

        void HandleKeyPress(const sf::Event::KeyEvent &key) {
            if (key.code == sf::Keyboard::Key::C) {
                state_.viewport = AppState::INITIAL_VIEWPORT;
                state_.auto_zoom_enabled = false;
            }

            if (key.code == sf::Keyboard::Key::X) {
                state_.auto_zoom_enabled = !state_.auto_zoom_enabled;
            }
        }

        void HandleMousePress(const sf::Event::MouseButtonEvent &mouse) {
            if (mouse.button == sf::Mouse::Left) {
                state_.left_mouse_pressed = true;
            } else if (mouse.button == sf::Mouse::Right) {
                state_.right_mouse_pressed = true;
            }
        }

        void HandleMouseRelease(const sf::Event::MouseButtonEvent &mouse) {
            if (mouse.button == sf::Mouse::Left) {
                state_.left_mouse_pressed = false;
            } else if (mouse.button == sf::Mouse::Right) {
                state_.right_mouse_pressed = false;
            }
        }

        void HandleAutoZoom() {
            const auto &vp = state_.viewport;
            int x = (AppState::AUTO_ZOOM_TARGET_X - vp.x_min) / vp.width() * render_settings_.width;
            int y = (AppState::AUTO_ZOOM_TARGET_Y - vp.y_min) / vp.height() * render_settings_.height;

            ZoomToPoint(x, y, true);
        }

        void ZoomToPoint(int pixel_x, int pixel_y, bool zoom_in, double factor = 0.8) {
            if (state_.zoom_clock.getElapsedTime().asMilliseconds() < ZOOM_INTERVAL_MS)
                return;
            else
                state_.zoom_clock.restart();

            const double target_x = state_.viewport.x_min +
                                    (static_cast<double>(pixel_x) / render_settings_.width) * state_.viewport.width();
            const double target_y = state_.viewport.y_min +
                                    (static_cast<double>(pixel_y) / render_settings_.height) * state_.viewport.height();

            const double zoom_factor = zoom_in ? factor : (1.0 / factor);
            const double new_width = state_.viewport.width() * zoom_factor;
            const double new_height = state_.viewport.height() * zoom_factor;

            /* Ваш код обновления state_ здесь  */
            state_.viewport.x_min = target_x - new_width / 2;
            state_.viewport.x_max = target_x + new_width / 2;
            state_.viewport.y_min = target_y - new_height / 2;
            state_.viewport.y_max = target_y + new_height / 2;

            state_.need_rerender = true;
        }
    };

    sf::RenderWindow &window_;
    RenderSettings render_settings_;
    AppState &state_;

    SfmlEventHandler(sf::RenderWindow &window, RenderSettings render_settings, AppState &state)
        : window_{window}, render_settings_{render_settings}, state_{state} {}

    /* Ваш код методов connect() и get_completion_signatures() здесь */
    template <typename Receiver>
    auto connect(Receiver &&receiver) const {
        return OperationState<std::decay_t<Receiver>>(std::forward<Receiver>(receiver), window_, render_settings_,
                                                      state_);
    }

    template <typename Env>
    auto get_completion_signatures(Env &&) const {
        return ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>{};
    }
};
