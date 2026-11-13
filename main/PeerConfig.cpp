#include "PeerConfig.hpp"

#include <algorithm>
#include <fcntl.h>
#include <unistd.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "nvs.h"
#include "nvs_flash.h"

namespace {

constexpr char NVS_NAMESPACE[] = "bass";
constexpr char NVS_PEER_KEY[] = "receiver_mac";

::esp_err_t saveReceiver(const bh::MAC &address) noexcept {
  ::nvs_handle_t handle{};
  auto result = ::nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
  if (result != ESP_OK) {
    return result;
  }
  result = ::nvs_set_blob(handle, NVS_PEER_KEY, address.data(), address.size());
  if (result == ESP_OK) {
    result = ::nvs_commit(handle);
  }
  ::nvs_close(handle);
  return result;
}

::esp_err_t addPeer(const bh::MAC &address) noexcept {
  ::esp_now_peer_info_t peer{};
  std::copy(address.begin(), address.end(), peer.peer_addr);
  peer.ifidx = ::WIFI_IF_STA;
  peer.channel = 0;
  peer.encrypt = false;
  return ::esp_now_add_peer(&peer);
}

} // namespace

bh::PeerConfig::PeerConfig(const bh::MAC &defaultAddress) noexcept
    : m_address(defaultAddress) {
  auto result = ::nvs_flash_init();
  if (result == ESP_ERR_NVS_NO_FREE_PAGES ||
      result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGW("bass", "Reinitializing NVS after %s; saved receiver MAC is lost",
             ::esp_err_to_name(result));
    ESP_ERROR_CHECK(::nvs_flash_erase());
    result = ::nvs_flash_init();
  }
  ESP_ERROR_CHECK(result);

  ::nvs_handle_t handle{};
  result = ::nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
  if (result == ESP_OK) {
    bh::MAC stored{};
    std::size_t size = stored.size();
    result = ::nvs_get_blob(handle, NVS_PEER_KEY, stored.data(), &size);
    if (result == ESP_OK && size == stored.size()) {
      m_address = stored;
    } else if (result == ESP_OK) {
      result = ESP_ERR_NVS_INVALID_LENGTH;
    }
    ::nvs_close(handle);
  }
  if (result == ESP_ERR_NVS_NOT_FOUND) {
    const auto saved = saveReceiver(m_address);
    if (saved != ESP_OK) {
      ESP_LOGW("bass", "Cannot save build-time receiver MAC: %s",
               ::esp_err_to_name(saved));
    }
  } else if (result != ESP_OK) {
    ESP_LOGW("bass", "Cannot load receiver MAC; using build default: %s",
             ::esp_err_to_name(result));
  }

  const int flags = ::fcntl(STDIN_FILENO, F_GETFL);
  ESP_ERROR_CHECK(
      flags >= 0 && ::fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) >= 0
          ? ESP_OK
          : ESP_FAIL);
  ESP_LOGI("bass", "Receiver MAC: " MACSTR, MAC2STR(m_address.data()));
  ESP_LOGI("bass", "Command: M AAAAAAAAAAAA (12 hex digits), then Enter");
}

void bh::PeerConfig::registerPeer() const noexcept {
  ESP_ERROR_CHECK(addPeer(m_address));
  ESP_LOGI("bass", "ESP-NOW receiver peer registered: " MACSTR,
           MAC2STR(m_address.data()));
}

void bh::PeerConfig::poll(bool hubActive) noexcept {
  const auto now = ::xTaskGetTickCount();
  constexpr auto interval = std::max<::TickType_t>(1, pdMS_TO_TICKS(10));
  if (static_cast<::TickType_t>(now - m_last_poll) < interval) {
    return;
  }
  m_last_poll = now;
  // Bound input processing so continuous serial traffic cannot starve playing.
  for (unsigned i = 0; i < 64; ++i) {
    char ch{};
    if (::read(STDIN_FILENO, &ch, 1) != 1) {
      break;
    }
    const auto command = m_reader.push(ch);
    switch (command.kind) {
    case bh::SerialCommand::Kind::Receiver:
      update(command.address, hubActive);
      break;
    case bh::SerialCommand::Kind::Invalid:
      ESP_LOGW("bass", "Invalid command; use M AAAAAAAAAAAA with one space, "
                       "then Enter");
      break;
    case bh::SerialCommand::Kind::None:
      break;
    }
  }
}

void bh::PeerConfig::update(const bh::MAC &address, bool hubActive) noexcept {
  const bool replacePeer = hubActive && address != m_address;
  if (replacePeer) {
    const auto result = addPeer(address);
    if (result != ESP_OK) {
      ESP_LOGE("bass", "Cannot register receiver MAC: %s",
               ::esp_err_to_name(result));
      return;
    }
  }

  const auto result = saveReceiver(address);
  if (result != ESP_OK) {
    if (replacePeer) {
      (void)::esp_now_del_peer(address.data());
    }
    ESP_LOGE("bass", "Cannot save receiver MAC: %s", ::esp_err_to_name(result));
    return;
  }
  if (replacePeer) {
    const auto removed = ::esp_now_del_peer(m_address.data());
    if (removed != ESP_OK) {
      ESP_LOGW("bass", "Cannot remove previous receiver: %s",
               ::esp_err_to_name(removed));
    }
  }
  m_address = address;
  ESP_LOGI("bass", "Receiver MAC saved: " MACSTR, MAC2STR(m_address.data()));
}
