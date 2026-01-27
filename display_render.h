#pragma once

#include "esphome.h"
#include "esphome/components/display/display_buffer.h"
#include "esphome/components/font/font.h"
#include "esphome/components/sensor/sensor.h"

// Use universal templates (T absorbs the pointer type) to avoid deduction issues
template <typename T_Font1, typename T_Font2, typename T_Font3, 
          typename T_Sens1, typename T_Sens2, typename T_Sens3, 
          typename T_Sens4, typename T_Sens5, typename T_Sens6>
inline void draw_main_page(esphome::display::Display &it, 
                    T_Font1 font_small, 
                    T_Font2 font_medium, 
                    T_Font3 icon_font, 
                    T_Sens1 fan_rpm, 
                    T_Sens2 temperature, 
                    T_Sens3 iaq, 
                    T_Sens4 humidity, 
                    T_Sens5 breath_voc, 
                    T_Sens6 pressure) {

    // --- VISUELLES LAYOUT (128x32 Pixel) ---
    // Globale Schriftarten & Farben Referenz:
    // font_small, font_medium, icon_font
    
    // 1. VERTICAK SEPARATOR (Mitte)
    it.line(60, 2, 60, 30); // Etwas Abstand oben/unten (2px)

    // ==========================================
    // LINKS: LÜFTER STATUS (0-60px)
    // ==========================================
    
    // Icon (Oben Links)
    it.print(0, 0, icon_font, "\U000F0210"); // mdi-fan
    
    if (fan_rpm->has_state()) {
      // RPM Wert (Groß & Fett wirkend durch font_medium)
      // Ausrichtung Rechtsbündig an der Trennlinie (-5px Padding)
      it.printf(55, 0, font_medium, esphome::display::TextAlign::TOP_RIGHT, "%.0f", fan_rpm->state);
      
      // Einheit "RPM" (Klein unter dem Wert)
      it.print(55, 14, font_small, esphome::display::TextAlign::TOP_RIGHT, "RPM");
      
      // --- RPM Progress Bar (Unten) ---
      // Bereich: x=2, y=26, w=56, h=4
      int bar_max_width = 56;
      float percentage = fan_rpm->state / 1850.0; // Max RPM Annahme
      if (percentage > 1.0) percentage = 1.0;
      int bar_fill = percentage * bar_max_width;
      
      // Rahmen (Optional, wirkt feiner ohne, aber testweise mit)
      it.rectangle(2, 27, bar_max_width, 4); // x, y, w, h
      // Füllung
      if (bar_fill > 0) {
          it.filled_rectangle(2, 27, bar_fill, 4);
      }

    } else {
      // Fallback wenn kein Wert
      it.print(30, 10, font_small, esphome::display::TextAlign::CENTER, "--");
    }

    // ==========================================
    // RECHTS: KLIMA DASHBOARD (62-128px)
    // ==========================================
    int x_start_right = 64;
    
    // --- ZEILE 1: TEMPERATUR (Immer sichtbar, "Header") ---
    // Thermometer Icon
    it.print(x_start_right, 0, icon_font, "\U000F050F"); 
    // Wert
    if (temperature->has_state()) {
       it.printf(128, 0, font_medium, esphome::display::TextAlign::TOP_RIGHT, "%.1f°", temperature->state);
    } else {
       it.print(128, 0, font_small, esphome::display::TextAlign::TOP_RIGHT, "--");
    }

    // --- ZEILE 2: CAROUSEL DATEN (Wechselt alle 4s) ---
    int slide = (millis() / 4000) % 4; // 4 Slides
    int y_row2 = 18; // Untere Zeile

    if (slide == 0) {
        // Slide 1: IAQ (Qualität)
        it.print(x_start_right, y_row2, icon_font, "\U000F002F"); // Filter Icon
        it.printf(128, y_row2, font_medium, esphome::display::TextAlign::TOP_RIGHT, "IAQ %.0f", iaq->state);
    } 
    else if (slide == 1) {
        // Slide 2: Luftfeuchtigkeit
        it.print(x_start_right, y_row2, icon_font, "\U000F058E"); // Tropfen
        it.printf(128, y_row2, font_medium, esphome::display::TextAlign::TOP_RIGHT, "%.1f%%", humidity->state);
    }
    else if (slide == 2) {
        // Slide 3: VOC
        it.print(x_start_right, y_row2+2, font_small, "VOC"); // Text Label, etwas tiefer
        it.printf(128, y_row2, font_medium, esphome::display::TextAlign::TOP_RIGHT, "%.1f", breath_voc->state);
    }
    else if (slide == 3) {
        // Slide 4: Luftdruck
        it.print(x_start_right, y_row2+2, font_small, "hPa");
        it.printf(128, y_row2, font_medium, esphome::display::TextAlign::TOP_RIGHT, "%.0f", pressure->state);
    }
}
