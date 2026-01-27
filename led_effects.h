#pragma once

#include "esphome.h"
#include "esphome/components/light/addressable_light.h"
#include <cmath>

// --- HELPER: HSL to RGB ---
inline esphome::Color hsl_to_rgb(float h, float s, float l) {
    float r, g, b;
    if (s == 0) {
        r = g = b = l; // achromatic
    } else {
        auto hue2rgb = [](float p, float q, float t) {
            if (t < 0) t += 1;
            if (t > 1) t -= 1;
            if (t < 1.0f/6.0f) return p + (q - p) * 6 * t;
            if (t < 1.0f/2.0f) return q;
            if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6;
            return p;
        };
        float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;
        r = hue2rgb(p, q, h + 1.0f/3.0f);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1.0f/3.0f);
    }
    return esphome::Color((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

// --- EFFECT: IAQ Plasma Ring ---
// LED 0: Center (IAQ Color)
// LED 1-6: Outer Ring (Rotating Plasma Pulse)
inline void iaq_plasma_ring_effect(esphome::light::AddressableLight &it, float iaq_value) {
    uint32_t now = millis();
    float time_f = now / 1000.0f;

    // 1. CENTER LED (0) - IAQ Color Gradient
    // IAQ 0-50 (Excellent) -> Green (Hue 0.33 / 120°)
    // IAQ 200+ (Bad) -> Red (Hue 0.0)
    // We map 0..250 to hue 0.33..0.0
    float iaq_norm = iaq_value / 250.0f;
    if (iaq_norm > 1.0f) iaq_norm = 1.0f;
    float iaq_hue = 0.33f * (1.0f - iaq_norm); // 0.33 down to 0
    it[0] = hsl_to_rgb(iaq_hue, 1.0f, 0.5f);

    // 2. OUTER RING (1-6) - Rotating Plasma Pulse
    int num_outer = it.size() - 1;
    for (int i = 1; i < it.size(); i++) {
        // Calculate position angle (0 to 2*PI)
        float angle = (float)(i - 1) / num_outer * 2.0f * M_PI;
        
        // Rotation effect: offset angle by time
        float rotation_speed = 2.0f; // rad/s
        float phase = angle - (time_f * rotation_speed);
        
        // Pulse brightness using sine wave (0.2 to 1.0)
        float pulse = (sinf(phase) + 1.0f) / 2.0f; // 0..1
        float brightness = 0.2f + (pulse * 0.8f);
        
        // Slowly shifting background color for the plasma effect
        float color_cycle_speed = 0.1f;
        float base_hue = fmodf(time_f * color_cycle_speed, 1.0f);
        
        it[i] = hsl_to_rgb(base_hue, 0.8f, brightness * 0.5f);
    }
}
