#pragma once

#include <iostream>

namespace Offsets {
    // regular Offsets (Updated with new values)
    inline uint64_t workspace = 0x178;
    inline uint64_t children = 0x70;          // Changed from 0x60 to 0x70
    inline uint64_t parent = 0x68;            // Changed from 0x50 to 0x68
    inline uint64_t class_descriptor = 0x18;
    inline uint64_t string = 0x8;
    inline uint64_t string_length = 0x10;
    inline uint64_t class_name = 0x8;
    inline uint64_t name = 0xB0;              // Changed from 0x80 to 0xB0
    inline uint64_t fake_data_model = 0x38;
    inline uint64_t real_data_model = 0x1B0;

    // part offsets (Updated with new values)
    inline uint64_t primitive = 0x148;
    inline uint64_t sizex = 0x1B0;            // Changed from 0x4F8 to 0x1B0 (PartSize)
    inline uint64_t sizey = 0x1B4;            // Changed from 0x4FC to 0x1B4
    inline uint64_t sizez = 0x1B8;            // Changed from 0x2B8 to 0x1B8

    // player stuff
    inline uint64_t local_player = 0x130;
    inline uint64_t character = 0x298;        // Note: This offset wasn't in the new list

    // New offsets from the dumped list
    inline uint64_t camera = 0x460;
    inline uint64_t view_matrix = 0x4B0;
    inline uint64_t camera_position = 0x11C;
    inline uint64_t camera_rotation = 0xF8;
    inline uint64_t health = 0x194;
    inline uint64_t max_health = 0x1B4;
    inline uint64_t walkspeed = 0x1D4;
    inline uint64_t jump_power = 0x1B0;
    inline uint64_t team = 0x280;
    inline uint64_t user_id = 0x2B8;
    inline uint64_t account_age = 0x2FC;
    inline uint64_t platform_name = 0xEB8;
    inline uint64_t part_position = 0xE4;
    inline uint64_t part_rotation = 0xC0;
    inline uint64_t part_velocity = 0xF0;
    inline uint64_t anchored = 0x1AE;
    inline uint64_t can_collide = 0x1AE;
    inline uint64_t transparency = 0xF0;
    inline uint64_t brick_color = 0x194;
    inline uint64_t material_type = 0x226;
    inline uint64_t humanoid_state = 0x8D8;
    inline uint64_t camera_fov = 0x160;
    inline uint64_t viewport_size = 0x2E8;

    // Note: The character offset (0x298) wasn't in the new dumped list
    // You may need to verify if this offset is still correct for the new client version
}