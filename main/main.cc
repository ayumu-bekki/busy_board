#include <memory>

#include "busy_board.h"
#include "logger.h"

extern "C" void app_main() {
  ESP_LOGI(TAG, "Start");

  BusyBoard busyBoard;
  busyBoard.Run();
}

// EOF
