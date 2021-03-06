#include "library/common/main_interface.h"

#include <atomic>

#include "library/common/engine.h"
#include "library/common/http/dispatcher.h"

// NOLINT(namespace-envoy)

static std::weak_ptr<Envoy::Engine> engine_;
static std::atomic<envoy_stream_t> current_stream_handle_{0};
static std::atomic<envoy_network_t> preferred_network_{ENVOY_NET_GENERIC};

envoy_stream_t init_stream(envoy_engine_t) { return current_stream_handle_++; }

envoy_status_t start_stream(envoy_stream_t stream, envoy_http_callbacks callbacks) {
  if (auto e = engine_.lock()) {
    return e->httpDispatcher().startStream(stream, callbacks);
  }
  return ENVOY_FAILURE;
}

envoy_status_t send_headers(envoy_stream_t stream, envoy_headers headers, bool end_stream) {
  if (auto e = engine_.lock()) {
    return e->httpDispatcher().sendHeaders(stream, headers, end_stream);
  }
  return ENVOY_FAILURE;
}

envoy_status_t send_data(envoy_stream_t stream, envoy_data data, bool end_stream) {
  if (auto e = engine_.lock()) {
    return e->httpDispatcher().sendData(stream, data, end_stream);
  }
  return ENVOY_FAILURE;
}

// TODO: implement.
envoy_status_t send_metadata(envoy_stream_t, envoy_headers) { return ENVOY_FAILURE; }

envoy_status_t send_trailers(envoy_stream_t stream, envoy_headers trailers) {
  if (auto e = engine_.lock()) {
    return e->httpDispatcher().sendTrailers(stream, trailers);
  }
  return ENVOY_FAILURE;
}

envoy_status_t reset_stream(envoy_stream_t stream) {
  if (auto e = engine_.lock()) {
    return e->httpDispatcher().resetStream(stream);
  }
  return ENVOY_FAILURE;
}

envoy_engine_t init_engine() {
  // TODO(goaway): return new handle once multiple engine support is in place.
  // https://github.com/lyft/envoy-mobile/issues/332
  return 1;
}

envoy_status_t set_preferred_network(envoy_network_t network) {
  preferred_network_.store(network);
  return ENVOY_SUCCESS;
}

void flush_stats() {
  if (auto e = engine_.lock()) {
    e->flushStats();
  }
}

/**
 * External entrypoint for library.
 */
envoy_status_t run_engine(envoy_engine_t, envoy_engine_callbacks callbacks, const char* config,
                          const char* log_level) {
  // This will change once multiple engine support is in place.
  // https://github.com/lyft/envoy-mobile/issues/332

  // The shared pointer created here will keep the engine alive until static destruction occurs.
  static auto strong_ref =
      std::make_shared<Envoy::Engine>(callbacks, config, log_level, preferred_network_);

  // The weak pointer we actually expose allows calling threads to atomically check if the engine
  // still exists and acquire a shared pointer to it - ensuring the engine persists at least for
  // the duration of the call.
  engine_ = strong_ref;
  return ENVOY_SUCCESS;
}
