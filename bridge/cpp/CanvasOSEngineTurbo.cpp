#include "CanvasOSEngineTurbo.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace canvasos {
namespace jsi = facebook::jsi;

namespace {

jsi::Function makeHostFunction(
    jsi::Runtime &rt,
    const char *name,
    unsigned int argCount,
    std::function<jsi::Value(jsi::Runtime &, const jsi::Value *, size_t)> impl) {
  return jsi::Function::createFromHostFunction(
      rt,
      jsi::PropNameID::forAscii(rt, name),
      argCount,
      [impl = std::move(impl)](
          jsi::Runtime &runtime,
          const jsi::Value &,
          const jsi::Value *args,
          size_t count) -> jsi::Value { return impl(runtime, args, count); });
}

double valueAsNumber(const jsi::Value &value) {
  return value.isNumber() ? value.asNumber() : 0;
}

std::string valueAsString(jsi::Runtime &rt, const jsi::Value &value) {
  return value.isString() ? value.asString(rt).utf8(rt) : "";
}

}  // namespace

CanvasOSEngineTurbo::CanvasOSEngineTurbo() = default;

jsi::Value CanvasOSEngineTurbo::get(
    jsi::Runtime &rt,
    const jsi::PropNameID &name) {
  auto prop = name.utf8(rt);

  if (prop == "initialize") {
    return makeHostFunction(rt, "initialize", 1, [this](jsi::Runtime &runtime, const jsi::Value *args, size_t count) {
      const auto dataDir = count > 0 ? valueAsString(runtime, args[0]) : "";
      return jsi::Value(doInitialize(dataDir));
    });
  }
  if (prop == "shutdown") {
    return makeHostFunction(rt, "shutdown", 0, [this](jsi::Runtime &runtime, const jsi::Value *, size_t) {
      doShutdown();
      return jsi::Value::undefined();
    });
  }
  if (prop == "tick") {
    return makeHostFunction(rt, "tick", 1, [this](jsi::Runtime &, const jsi::Value *args, size_t count) {
      return jsi::Value(doTick(count > 0 ? static_cast<int>(valueAsNumber(args[0])) : 0));
    });
  }
  if (prop == "getHash") {
    return makeHostFunction(rt, "getHash", 0, [this](jsi::Runtime &runtime, const jsi::Value *, size_t) {
      return jsi::String::createFromUtf8(runtime, doGetHash());
    });
  }
  if (prop == "getStatus") {
    return makeHostFunction(rt, "getStatus", 0, [this](jsi::Runtime &runtime, const jsi::Value *, size_t) {
      return jsi::String::createFromUtf8(runtime, doGetStatus());
    });
  }
  if (prop == "setViewport") {
    return makeHostFunction(rt, "setViewport", 5, [this](jsi::Runtime &, const jsi::Value *args, size_t count) {
      if (count == 5) {
        doSetViewport(
            static_cast<int>(valueAsNumber(args[0])),
            static_cast<int>(valueAsNumber(args[1])),
            static_cast<int>(valueAsNumber(args[2])),
            static_cast<int>(valueAsNumber(args[3])),
            static_cast<int>(valueAsNumber(args[4])));
      }
      return jsi::Value::undefined();
    });
  }
  if (prop == "setVisMode") {
    return makeHostFunction(rt, "setVisMode", 1, [this](jsi::Runtime &, const jsi::Value *args, size_t count) {
      doSetVisMode(count > 0 ? static_cast<int>(valueAsNumber(args[0])) : 0);
      return jsi::Value::undefined();
    });
  }
  if (prop == "renderFrame") {
    return makeHostFunction(rt, "renderFrame", 0, [this](jsi::Runtime &, const jsi::Value *, size_t) {
      doRenderFrame();
      return jsi::Value::undefined();
    });
  }
  if (prop == "getFrameBuffer") {
    return makeHostFunction(rt, "getFrameBuffer", 0, [this](jsi::Runtime &runtime, const jsi::Value *, size_t) {
      return jsi::String::createFromUtf8(runtime, doGetFrameBuffer());
    });
  }
  if (prop == "getCell") {
    return makeHostFunction(rt, "getCell", 2, [this](jsi::Runtime &runtime, const jsi::Value *args, size_t count) {
      return jsi::String::createFromUtf8(
          runtime,
          doGetCell(
              count > 0 ? static_cast<int>(valueAsNumber(args[0])) : 0,
              count > 1 ? static_cast<int>(valueAsNumber(args[1])) : 0));
    });
  }
  if (prop == "exec") {
    return makeHostFunction(rt, "exec", 1, [this](jsi::Runtime &runtime, const jsi::Value *args, size_t count) {
      return jsi::String::createFromUtf8(runtime, doExec(count > 0 ? valueAsString(runtime, args[0]) : ""));
    });
  }
  if (prop == "exportBMP") {
    return makeHostFunction(rt, "exportBMP", 1, [this](jsi::Runtime &runtime, const jsi::Value *args, size_t count) {
      return jsi::Value(doExportBMP(count > 0 ? valueAsString(runtime, args[0]) : ""));
    });
  }
  if (prop == "query") {
    return makeHostFunction(rt, "query", 1, [this](jsi::Runtime &runtime, const jsi::Value *args, size_t count) {
      return jsi::String::createFromUtf8(runtime, doQuery(count > 0 ? valueAsString(runtime, args[0]) : ""));
    });
  }

  return jsi::Value::undefined();
}

std::vector<jsi::PropNameID> CanvasOSEngineTurbo::getPropertyNames(jsi::Runtime &rt) {
  static const char *names[] = {
      "initialize", "shutdown", "tick",      "getHash", "getStatus", "setViewport",
      "setVisMode", "renderFrame", "getFrameBuffer", "getCell", "exec", "exportBMP", "query"};
  std::vector<jsi::PropNameID> properties;
  properties.reserve(sizeof(names) / sizeof(names[0]));
  for (const auto *prop : names) {
    properties.push_back(jsi::PropNameID::forAscii(rt, prop));
  }
  return properties;
}

bool CanvasOSEngineTurbo::doInitialize(const std::string &dataDir) {
  doShutdown();

  dataDir_ = dataDir;
  std::memset(cells_, 0, sizeof(cells_));
  std::memset(gates_, 0, sizeof(gates_));
  std::memset(active_, 0, sizeof(active_));

  engctx_init(&ctx_, cells_, CANVAS_W * CANVAS_H, gates_, active_, nullptr);
  tervas_init(&tv_);
  ptl_state_init(&ptl_, ORIGIN_X, ORIGIN_Y);
  proctable_init(&proctable_, &ctx_);
  pipe_table_init(&pipetable_);
  syscall_init();
  vm_bridge_init(&proctable_, &pipetable_);
  fd_table_init();
  fd_set_pipe_table(&pipetable_);
  shell_init(&shell_, &proctable_, &pipetable_, &ctx_);

  if (!dataDir_.empty()) {
    const auto sessionPath = dataDir_ + "/session.cvp";
    cvp_load_ctx(
        &ctx_,
        sessionPath.c_str(),
        false,
        CVP_LOCK_SKIP,
        CVP_LOCK_SKIP,
        CVP_CONTRACT_HASH_V1);
  }

  engctx_tick(&ctx_);

  gui_init(&gui_, 64 * 4, 64 * 4);
  bridge_init(&bridge_, &ctx_, &gui_);
  bridge_set_viewport(&bridge_, 480, 480, 64, 64, 4);
  bridge_set_vis_mode(&bridge_, CELL_VIS_ABGR);
  bridge_render_frame(&bridge_);
  initialized_ = true;
  return true;
}

void CanvasOSEngineTurbo::doShutdown() {
  if (gui_.framebuffer.pixels != nullptr) {
    gui_free(&gui_);
  }
  initialized_ = false;
  dataDir_.clear();
  lastExecOutput_.clear();
  std::memset(&ctx_, 0, sizeof(ctx_));
  std::memset(&tv_, 0, sizeof(tv_));
  std::memset(&ptl_, 0, sizeof(ptl_));
  std::memset(&proctable_, 0, sizeof(proctable_));
  std::memset(&pipetable_, 0, sizeof(pipetable_));
  std::memset(&shell_, 0, sizeof(shell_));
  std::memset(&gui_, 0, sizeof(gui_));
  std::memset(&bridge_, 0, sizeof(bridge_));
}

int CanvasOSEngineTurbo::doTick(int n) {
  if (!initialized_) {
    return 0;
  }
  const auto steps = std::max(0, n);
  for (int i = 0; i < steps; ++i) {
    engctx_tick(&ctx_);
  }
  return static_cast<int>(ctx_.tick);
}

std::string CanvasOSEngineTurbo::doGetHash() const {
  char hex[9];
  std::snprintf(hex, sizeof(hex), "%08x", dk_canvas_hash(cells_, CANVAS_W * CANVAS_H));
  return std::string(hex);
}

std::string CanvasOSEngineTurbo::doGetStatus() const {
  std::ostringstream stream;
  stream << "{"
         << "\"tick\":" << ctx_.tick << ","
         << "\"hash\":\"" << doGetHash() << "\","
         << "\"branchId\":" << ctx_.now.page_id << ","
         << "\"openGates\":" << countOpenGates() << ","
         << "\"visMode\":" << static_cast<int>(bridge_.vis_mode) << ","
         << "\"pcX\":" << ptl_.cx << ","
         << "\"pcY\":" << ptl_.cy
         << "}";
  return stream.str();
}

void CanvasOSEngineTurbo::doSetViewport(int x, int y, int w, int h, int cellPx) {
  if (!initialized_) {
    return;
  }
  gui_free(&gui_);
  gui_init(&gui_, std::max(0, w * cellPx), std::max(0, h * cellPx));
  bridge_init(&bridge_, &ctx_, &gui_);
  bridge_set_viewport(
      &bridge_,
      static_cast<uint16_t>(x),
      static_cast<uint16_t>(y),
      static_cast<uint16_t>(w),
      static_cast<uint16_t>(h),
      static_cast<uint8_t>(cellPx));
  bridge_render_frame(&bridge_);
}

void CanvasOSEngineTurbo::doSetVisMode(int mode) {
  if (!initialized_) {
    return;
  }
  bridge_set_vis_mode(&bridge_, static_cast<CellVisMode>(mode));
}

void CanvasOSEngineTurbo::doRenderFrame() {
  if (!initialized_) {
    return;
  }
  bridge_render_frame(&bridge_);
}

std::string CanvasOSEngineTurbo::doGetFrameBuffer() const {
  std::ostringstream stream;
  const auto byteLength =
      static_cast<uint32_t>(gui_.framebuffer.w * gui_.framebuffer.h * sizeof(GuiPixel));
  stream << "{"
         << "\"ptr\":" << static_cast<double>(reinterpret_cast<uintptr_t>(gui_.framebuffer.pixels)) << ","
         << "\"width\":" << gui_.framebuffer.w << ","
         << "\"height\":" << gui_.framebuffer.h << ","
         << "\"byteLength\":" << byteLength
         << "}";
  return stream.str();
}

std::string CanvasOSEngineTurbo::doGetCell(int x, int y) const {
  if (x < 0 || y < 0 || x >= CANVAS_W || y >= CANVAS_H) {
    return R"({"a":0,"b":0,"g":0,"r":0})";
  }
  const auto &cell = cells_[static_cast<size_t>(y) * CANVAS_W + static_cast<size_t>(x)];
  std::ostringstream stream;
  stream << "{"
         << "\"a\":" << cell.A << ","
         << "\"b\":" << static_cast<int>(cell.B) << ","
         << "\"g\":" << static_cast<int>(cell.G) << ","
         << "\"r\":" << static_cast<int>(cell.R)
         << "}";
  return stream.str();
}

std::string CanvasOSEngineTurbo::doExec(const std::string &command) {
  if (!initialized_) {
    return "ERROR: not initialized";
  }

  fd_stdout_clear();
  const auto rc = shell_exec_line(&shell_, &ctx_, command.c_str());
  uint8_t outbuf[4096];
  const auto len = fd_stdout_get(outbuf, sizeof(outbuf));
  lastExecOutput_ = std::string(reinterpret_cast<char *>(outbuf), reinterpret_cast<char *>(outbuf) + len);
  if (lastExecOutput_.empty()) {
    lastExecOutput_ = rc == 0 ? "OK" : "ERROR";
  }
  return lastExecOutput_;
}

bool CanvasOSEngineTurbo::doExportBMP(const std::string &path) {
  if (!initialized_) {
    return false;
  }
  bridge_render_frame(&bridge_);
  return gui_bmp_write(&gui_.framebuffer, path.c_str()) == 0;
}

std::string CanvasOSEngineTurbo::doQuery(const std::string &request) const {
  if (request.find("\"getStatus\"") != std::string::npos) {
    return std::string(R"({"ok":true,"data":)") + doGetStatus() + "}";
  }
  return R"({"ok":false,"error":"unknown method"})";
}

int CanvasOSEngineTurbo::countOpenGates() const {
  int open = 0;
  for (int i = 0; i < TILE_COUNT; ++i) {
    if (gates_[i] == GATE_OPEN) {
      ++open;
    }
  }
  return open;
}

void installCanvasOSEngine(jsi::Runtime &rt) {
  auto object = std::make_shared<CanvasOSEngineTurbo>();
  auto hostObject = jsi::Object::createFromHostObject(rt, object);
  rt.global().setProperty(rt, "CanvasOSEngine", std::move(hostObject));
}

}  // namespace canvasos
