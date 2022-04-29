/*
 * OpenixCard.cpp
 * Copyright (c) 2022, YuzukiTsuru <GloomyGhost@GloomyGhost.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * See README and LICENSE for more details.
 */

#include <ColorCout.hpp>
#include <argparse/argparse.hpp>
#include <filesystem>

#include "LOG.h"
#include "exception.h"
#include "config.h"
#include "FEX2CFG.h"
#include "Genimage.h"

extern "C" {
#include "OpenixIMG.h"
}


#include "OpenixCard.h"

OpenixCard::OpenixCard(int argc, char **argv) {
    parse_args(argc, argv);

    LOG::INFO("Input file: " + this->input_file + " Now converting...");
    unpack_target_image();

    LOG::INFO("Convert Done! Prasing the partition tables...");

    if (this->is_dump) {
        dump_and_clean();
    } else if (this->is_unpack) {
        LOG::INFO("Unpack Done! Your image file at " + this->temp_file_path);
    } else {
        save_cfg_file();
    }
}

void OpenixCard::parse_args(int argc, char **argv) {
    argparse::ArgumentParser parser("OpenixCard", []() {
        if (std::string(PROJECT_GIT_HASH).empty())
            return PROJECT_VER;
        else
            return PROJECT_GIT_HASH;
    }());
    show_logo();

    parser.add_argument("-u", "--unpack")
            .help("Unpack Allwinner Image to folder")
            .default_value(false)
            .implicit_value(true);
    parser.add_argument("-d", "--dump")
            .help("Convert Allwinner image to regular image")
            .default_value(false)
            .implicit_value(true);
    parser.add_argument("-c", "--cfg")
            .help("Get Allwinner image partition table cfg file")
            .default_value(false)
            .implicit_value(true);
    parser.add_argument("-i", "--input")
            .help("Input Allwinner image file")
            .required();
    parser.add_argument("-o", "--output")
            .help("Output file path")
            .default_value("output");

    try {
        // parser args
        parser.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err) {
        std::cout << cc::red << "INPUT ERROR: " << err.what() << cc::reset << std::endl; // args invalid
        std::cout << parser; // show help
        std::exit(1);
    }

    this->input_file = parser.get<std::string>("input");

    // if input file path is absolute path, convert to relative path, #1
    std::filesystem::path input_path(input_file);
    if (input_path.is_absolute()) {
        this->temp_file_path = std::string(input_path.relative_path()) + ".dump";
    } else {
        this->temp_file_path = input_file + ".dump";
    }

    this->output_file_path = parser.get<std::string>("output");
    this->is_unpack = parser.get<bool>("unpack");
    this->is_dump = parser.get<bool>("dump");
    this->is_cfg = parser.get<bool>("cfg");

    if (!this->is_unpack && !this->is_dump && !this->is_cfg) {
        LOG::ERROR("No action selected, please select one of the following actions: -u, -d, -c");
        std::cout << parser; // show help
        std::exit(1);
    }
}

void OpenixCard::show_logo() {
    std::cout << cc::blue <<
              " _____             _     _____           _ \n"
              "|     |___ ___ ___|_|_ _|     |___ ___ _| |\n"
              "|  |  | . | -_|   | |_'_|   --| .'|  _| . |\n"
              "|_____|  _|___|_|_|_|_,_|_____|__,|_| |___|\n"
              "      |_| Version: " << PROJECT_GIT_HASH
              << cc::yellow <<
              "\nCopyright (c) 2022, YuzukiTsuru <GloomyGhost@GloomyGhost.com>\n"
              << cc::reset << std::endl;
}

void OpenixCard::unpack_target_image() {
    // dump the packed image
    crypto_init();
    unpack_image(this->input_file.c_str(), this->temp_file_path.c_str());
}

void OpenixCard::dump_and_clean() {
    FEX2CFG fex2Cfg(this->temp_file_path);
    auto target_cfg_path = fex2Cfg.save_file(this->temp_file_path);
    auto image_name = fex2Cfg.get_image_name();
    // generate the image
    LOG::INFO("Prase Done! Generating target image...");

    Genimage genimage(target_cfg_path, this->temp_file_path, this->output_file_path);

    // check genimage result
    if (genimage.get_status() != 0) {
        LOG::ERROR("Generate image failed!");
        std::exit(1);
    }

    LOG::INFO("Generate Done! Your image file at " + this->output_file_path + " Cleaning up...");
    std::filesystem::remove_all(this->temp_file_path);
}

void OpenixCard::save_cfg_file() {
    FEX2CFG fex2Cfg(this->temp_file_path);
    auto target_cfg_path = fex2Cfg.save_file(this->output_file_path);
    std::filesystem::remove_all(this->temp_file_path);
    LOG::INFO("Prase Done! Your cfg file at " + target_cfg_path);
}