#pragma once

#include "mandelbrot_fractal_utils.hpp"
#include "types_sfml.hpp"
#include <print>

#include <stdexec/execution.hpp>

using namespace std::chrono_literals;
namespace ex = stdexec;

namespace mandelbrot {
static auto MakeComputeSender(FrameBuffer *fb, bool need_rerender, RenderSettings settings, ViewPort viewport) {
    auto time_counter = std::make_shared<AvrTimeCounter>();
    return ex::just(fb) | ex::then([need_rerender, settings, viewport, time_counter](FrameBuffer *fb) {
               if (!need_rerender)
                   return fb;

               time_counter->Start();

               for (auto x = 0; x < fb->width; x++)
                   for (auto y = 0; y < fb->height; y++) {
                       auto cmplx = Pixel2DToComplex(x, y, viewport, settings.width, settings.height);
                       auto iters = CalculateIterationsForPoint(cmplx, settings.max_iterations, settings.escape_radius);
                       auto color = IterationsToColor(iters, settings.max_iterations);

                       auto index = (y * fb->width + x) * 4;
                       fb->rgba[index] = color.r;
                       fb->rgba[index + 1] = color.g;
                       fb->rgba[index + 2] = color.b;
                       fb->rgba[index + 3] = 255;
                   }
               return fb;
           }) |
           ex::then([time_counter](FrameBuffer *fb) {
               time_counter->End();
               if (time_counter->Count() % 10 == 0) {
                   std::println("\nAverage compute time: {} ms over {} frames", time_counter->GetAvr(),
                                time_counter->Count());
               }
               time_counter->Reset();
               return fb;
           });
}

}  // namespace mandelbrot
