#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "Loader.hpp"
#include "utils.hpp"
#include "ZipManager.hpp"

Loader::Loader() {
    this->InitLogger();

    this->version = new Version();
    version->Check();

    spdlog::info("Metaloader {}", VERSION);
    spdlog::info("Build: {} {}", BUILD_DATE, BUILD_TIME);
    spdlog::info("Current process: {}", Utils::GetCurrentProcessName());

    if (!this->zip) this->zip = new ZipManager();
    if (!this->settings) this->settings = new Settings();
    this->hook = new GameHook(this->g_isRun);
    if (!this->g_isRun) return;

    this->g_isRun = hook->CreateHook();

    this->render = new LoaderUI(this->hook->mlo);
}

Loader::~Loader() {
    delete this->hook;
    delete this->render;
    delete this->version;
    delete this->settings;
    delete this->zip;
    this->g_isRun = false;

    if (this->g_isLoggerReady) {
        spdlog::info("Metaloader unloading");
        spdlog::default_logger()->flush();
        spdlog::shutdown();
    }
}

void Loader::InitLogger() {
    auto logger = spdlog::basic_logger_mt<spdlog::async_factory>(
        "logger",
        "metaloader/metaloader.log.txt",
        true
    );
    spdlog::set_default_logger(logger);

    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::flush_every(std::chrono::seconds(1));

    this->g_isLoggerReady = true;
}
