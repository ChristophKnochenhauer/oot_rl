#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "soh_lib.h"
#include "libultraship/libultra/controller.h"

namespace py = pybind11;

// C-API convention: 0 = success, negative rc = error. Surface any failure as a
// Python RuntimeError carrying the rc so it stays diagnosable.
static void check(int rc, const char* what) {
    if (rc != 0) throw std::runtime_error(std::string(what) + " failed (rc=" + std::to_string(rc) + ")");
}

PYBIND11_MODULE(oot_rl, m) {
    m.doc() = "Native bindings for the SoH RL environment";

    m.def("set_headless", [](bool on) { SoH_SetHeadless(on ? 1 : 0); }, py::arg("on") = true);

    m.def("set_app_path", [](const std::string& p) {
        setenv("LUS_APP_BUNDLE_PATH", p.c_str(), 1);
    }, py::arg("path"));

    m.def("init", [](std::vector<std::string> args) {
        std::vector<char*> argv;
        argv.reserve(args.size());
        for (auto& s : args) argv.push_back(const_cast<char*>(s.c_str()));
        SoH_Init((int)argv.size(), argv.data());
    }, py::arg("args") = std::vector<std::string>{"oot_env"});

    m.def("shutdown", []() { SoH_Shutdown(); });

    m.def("step_frame", []() { SoH_StepFrame(); });

    m.def("warp_to",
        [](int entrance_id, int room_num, float x, float y, float z, int rot_y) {
            check(SoH_WarpTo(entrance_id, room_num, x, y, z, rot_y), "warp_to");
        },
        py::arg("entrance_id"),
        py::arg("room_num") = 0,
        py::arg("x") = 0.0f,
        py::arg("y") = 0.0f,
        py::arg("z") = 0.0f,
        py::arg("rot_y") = 0);

    m.def("warp_to_entrance", [](int entrance_id, int link_age) {
        check(SoH_WarpToEntrance(entrance_id, link_age), "warp_to_entrance");
    }, py::arg("entrance_id"), py::arg("link_age") = -1);

    m.def("enable_debug_view",
        [](uint32_t width, uint32_t height) { SoH_EnableDebugView(width, height); },
        py::arg("width") = 640, py::arg("height") = 480);
    m.def("disable_debug_view", []() { SoH_DisableDebugView(); });

    m.def("set_input",
        [](int port, uint32_t buttons, int stick_x, int stick_y,
                                       int right_stick_x, int right_stick_y) {
            SoH_Input in = {};
            in.buttons        = buttons;
            in.stick_x        = (int8_t)stick_x;
            in.stick_y        = (int8_t)stick_y;
            in.right_stick_x  = (int8_t)right_stick_x;
            in.right_stick_y  = (int8_t)right_stick_y;
            SoH_SetInput(port, &in);
        },
        py::arg("port") = 0,
        py::arg("buttons") = 0,
        py::arg("stick_x") = 0,
        py::arg("stick_y") = 0,
        py::arg("right_stick_x") = 0,
        py::arg("right_stick_y") = 0);

    m.def("clear_input", [](int port) { SoH_ClearInput(port); }, py::arg("port") = 0);

    m.attr("BTN_A")     = (uint32_t)BTN_A;
    m.attr("BTN_B")     = (uint32_t)BTN_B;
    m.attr("BTN_START") = (uint32_t)BTN_START;
    m.attr("BTN_Z")     = (uint32_t)BTN_Z;
    m.attr("BTN_L")     = (uint32_t)BTN_L;
    m.attr("BTN_R")     = (uint32_t)BTN_R;
    m.attr("BTN_CUP")    = (uint32_t)BTN_CUP;
    m.attr("BTN_CDOWN")  = (uint32_t)BTN_CDOWN;
    m.attr("BTN_CLEFT")  = (uint32_t)BTN_CLEFT;
    m.attr("BTN_CRIGHT") = (uint32_t)BTN_CRIGHT;
    m.attr("BTN_DUP")    = (uint32_t)BTN_DUP;
    m.attr("BTN_DDOWN")  = (uint32_t)BTN_DDOWN;
    m.attr("BTN_DLEFT")  = (uint32_t)BTN_DLEFT;
    m.attr("BTN_DRIGHT") = (uint32_t)BTN_DRIGHT;

    py::class_<SoH_GameState>(m, "GameState")
        .def(py::init<>())
        .def_readonly("valid",      &SoH_GameState::valid)
        .def_readonly("pos_x",      &SoH_GameState::pos_x)
        .def_readonly("pos_y",      &SoH_GameState::pos_y)
        .def_readonly("pos_z",      &SoH_GameState::pos_z)
        .def_readonly("rot_x",      &SoH_GameState::rot_x)
        .def_readonly("rot_y",      &SoH_GameState::rot_y)
        .def_readonly("rot_z",      &SoH_GameState::rot_z)
        .def_readonly("health",     &SoH_GameState::health)
        .def_readonly("max_health", &SoH_GameState::max_health)
        .def_readonly("magic",      &SoH_GameState::magic)
        .def_readonly("rupees",     &SoH_GameState::rupees)
        .def_readonly("scene_id",   &SoH_GameState::scene_id)
        .def_readonly("day_time",   &SoH_GameState::day_time)
        .def("__repr__", [](const SoH_GameState& s) {
            return "GameState(valid=" + std::to_string(s.valid) +
                   " scene=" + std::to_string(s.scene_id) +
                   " pos=(" + std::to_string(s.pos_x) + "," + std::to_string(s.pos_y) + "," + std::to_string(s.pos_z) + ")" +
                   " hp=" + std::to_string(s.health) + "/" + std::to_string(s.max_health) + ")";
        });

    m.def("get_game_state", []() {
        SoH_GameState s = {};
        check(SoH_GetGameState(&s), "get_game_state");
        return s;
    });

    m.def("save_state", [](unsigned int slot) {
        check(SoH_SaveState(slot), "save_state");
    }, py::arg("slot") = 0);

    m.def("load_state", [](unsigned int slot) {
        check(SoH_LoadState(slot), "load_state");
    }, py::arg("slot") = 0);

    m.def("snapshot_actor_counts", []() { SoH_SnapshotActorCounts(); });
    m.def("restore_actor_counts",  []() { SoH_RestoreActorCounts();  });

    m.def("enable_frame_capture",  []() { SoH_EnableFrameCapture();  });
    m.def("disable_frame_capture", []() { SoH_DisableFrameCapture(); });

    m.def("get_frame_dimensions", []() {
        int w = 0, h = 0;
        check(SoH_GetFrameDimensions(&w, &h), "get_frame_dimensions");
        return py::make_tuple(w, h);
    });

    m.def("get_frame", []() {
        int w = 0, h = 0;
        check(SoH_GetFrameDimensions(&w, &h), "get_frame_dimensions");
        py::array_t<uint8_t> arr({(py::ssize_t)h, (py::ssize_t)w, (py::ssize_t)3});
        check(SoH_GetFrame(arr.mutable_data()), "get_frame");
        return arr;
    });
}