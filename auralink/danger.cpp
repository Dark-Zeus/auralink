// if normal, return transparent color
// if warning, return yellow with medium opacity
// if danger, return red with medium opacity

#pragma once
#include "danger.h"

ColorOpacity getDangerColorAirQuality(float aqi){
    if (aqi <= 50.0f) {
        return {lv_color_hex(0x00E400), 0}; // Normal
    } else if (aqi <= 100.0f) {
        return {lv_color_hex(0xFFFF00), 80}; // Moderate
    } else if (aqi <= 150.0f) {
        return {lv_color_hex(0xFFA500), 80}; // Unhealthy for Sensitive Groups
    } else if (aqi <= 200.0f) {
        return {lv_color_hex(0xFF0000), 80}; // Unhealthy
    } else if (aqi <= 300.0f) {
        return {lv_color_hex(0x800080), 80}; // Very Unhealthy
    } else {
        return {lv_color_hex(0x800000), 80}; // Hazardous
    }
}

ColorOpacity getDangerColorIllumination(float lux){
    if (lux >= 300.0f) {
        return {lv_color_hex(0x000000), 0};  // Normal
    } else if (lux >= 100.0f) {
        return {lv_color_hex(0xFFFF00), 80};  // Warning
    } else {
        return {lv_color_hex(0xFF0000), 80};  // Danger
    }
}

ColorOpacity getDangerColorHumidity(float rh){
    if (rh >= 30.0f && rh <= 60.0f) {
        return {lv_color_hex(0x000000), 0};  // Normal
    } else if ((rh >= 20.0f && rh < 30.0f) || (rh > 60.0f && rh <= 70.0f)) {
        return {lv_color_hex(0xFFFF00), 80};  // Warning
    } else {
        return {lv_color_hex(0xFF0000), 80};  // Danger
    }
}

ColorOpacity getDangerColorTemperature(float celsius){
    if (celsius >= 20.0f && celsius <= 25.0f) {
        return {lv_color_hex(0x000000), 0};  // Normal
    } else if ((celsius >= 15.0f && celsius < 20.0f) || (celsius > 25.0f && celsius <= 30.0f)) {
        return {lv_color_hex(0xFFFF00), 80}; // Warning
    } else {
        return {lv_color_hex(0xFF0000), 80}; // Danger
    }
}

ColorOpacity getDangerColorPressure(float hpa){
    if (hpa >= 1013.0f && hpa <= 1020.0f) {
        return {lv_color_hex(0x000000), 0}; // Normal
    } else if ((hpa >= 1000.0f && hpa < 1013.0f) || (hpa > 1020.0f && hpa <= 1030.0f)) {
        return {lv_color_hex(0xFFFF00), 80}; // Warning
    } else {
        return {lv_color_hex(0xFF0000), 80}; // Danger
    }
}

ColorOpacity getDangerColorLoudness(float db){
    if (db <= 70.0f) {
        return {lv_color_hex(0x000000), 0}; // Normal
    } else if (db <= 85.0f) {
        return {lv_color_hex(0xFFFF00), 80}; // Warning
    } else {
        return {lv_color_hex(0xFF0000), 80}; // Danger
    }
}

ColorOpacity getDangerColorUVIndex(float uvi){
    if (uvi <= 2.0f) {
        return {lv_color_hex(0x000000), 0}; // Normal
    } else if (uvi <= 5.0f) {
        return {lv_color_hex(0xFFFF00), 80}; // Moderate
    } else if (uvi <= 7.0f) {
        return {lv_color_hex(0xFFA500), 80}; // High
    } else if (uvi <= 10.0f) {
        return {lv_color_hex(0xFF0000), 80}; // Very High
    } else {
        return {lv_color_hex(0x800080), 80}; // Extreme
    }
}
