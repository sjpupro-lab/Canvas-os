#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <string>
#include <vector>

extern "C" {
#include "canvas_determinism.h"
#include "canvasos_bridge.h"
#include "canvasos_engine_ctx.h"
#include "canvasos_fd.h"
#include "canvasos_gate_ops.h"
#include "canvasos_gui.h"
#include "canvasos_shell.h"
#include "canvasos_types.h"
#include "canvasos_proc.h"
#include "canvasos_pipe.h"
#include "cvp_io.h"
#include "gui_engine_bridge.h"
#include "sjptl.h"
#include "tervas_core.h"
#include "canvasos_syscall.h"
}

namespace canvasos {

class CanvasOSEngineTurbo : public facebook::jsi::HostObject {
 public:
  CanvasOSEngineTurbo();
  ~CanvasOSEngineTurbo() override = default;

  facebook::jsi::Value get(
      facebook::jsi::Runtime &rt,
      const facebook::jsi::PropNameID &name) override;
  std::vector<facebook::jsi::PropNameID> getPropertyNames(
      facebook::jsi::Runtime &rt) override;

 private:
  bool initialized_ = false;
  std::string dataDir_;
  std::string lastExecOutput_;
  Cell cells_[CANVAS_W * CANVAS_H]{};
  GateState gates_[TILE_COUNT]{};
  uint8_t active_[TILE_COUNT]{};
  EngineContext ctx_{};
  Tervas tv_{};
  PtlState ptl_{};
  ProcTable proctable_{};
  PipeTable pipetable_{};
  Shell shell_{};
  GuiContext gui_{};
  GuiEngineBridge bridge_{};

  bool doInitialize(const std::string &dataDir);
  void doShutdown();
  int doTick(int n);
  std::string doGetHash() const;
  std::string doGetStatus() const;
  void doSetViewport(int x, int y, int w, int h, int cellPx);
  void doSetVisMode(int mode);
  void doRenderFrame();
  std::string doGetFrameBuffer() const;
  std::string doGetCell(int x, int y) const;
  std::string doExec(const std::string &command);
  bool doExportBMP(const std::string &path);
  std::string doQuery(const std::string &request) const;
  int countOpenGates() const;
};

void installCanvasOSEngine(facebook::jsi::Runtime &rt);

}  // namespace canvasos
