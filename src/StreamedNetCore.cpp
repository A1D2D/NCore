#include "StreamedNetCore.h"

namespace SN {
   /*CONTEXT*/
   Context::Context() : state(std::make_shared<State>()) {}

   void Context::use() {
      if (!state) return;
      state->usageCount++;
   }

   void Context::release() {
      if (!state) return;
      if (state->usageCount == 0) {
         std::cerr << "usage cant be dicreased already 0" << std::endl;
         return;
      }
      state->usageCount--;
   }

   size_t Context::usage() const {
      if (!state) return 0;
      return state->usageCount.load();
   }

   Context::CallbackHandle Context::addCallback(std::function<void()> callback) {
      if (!state) return {};
      CallbackPtr callbackPtr = std::make_shared<std::function<void()>>(std::move(callback));
      std::lock_guard lock(state->callbackMutex);
      state->callbacks.emplace_back(callbackPtr);
      return callbackPtr;
   }

   void Context::removeCallback(const CallbackHandle& handle) {
      if (!state) return;

      auto callback = handle.lock();

      if (!callback) return;

      std::lock_guard lock(state->callbackMutex);
      std::erase_if(state->callbacks, [&](const CallbackPtr& entry) { return entry == callback; });
   }

   void Context::poll() {
      if (!state) {
         std::cerr << "failed to poll state is null ptr" << std::endl;
         return;
      }
      std::vector<CallbackPtr> tempCallbacks;
      {
         std::lock_guard lock(state->callbackMutex);
         tempCallbacks = state->callbacks;
      }
      state->io.poll();
      for (CallbackPtr& callback : tempCallbacks) {
         if (!callback) continue;
         (*callback)();
      }
   }

   // UseGuard
   Context::UsageGuard::UsageGuard(Context context) : context(std::move(context)) { this->context.use(); }

   Context::UsageGuard& Context::UsageGuard::operator=(UsageGuard&& other) noexcept {
      if (this != &other) {
         release();
         context = std::move(other.context);
         active = other.active;
         other.active = false;
      }
      return *this;
   }

   Context::UsageGuard::UsageGuard(UsageGuard&& other) noexcept : context(std::move(other.context)), active(other.active) { other.active = false; }

   void Context::UsageGuard::release() {
      if (active) {
         context.release();
         active = false;
      }
   }

   Context::UsageGuard::~UsageGuard() { release(); }

   // Callback
   Context::Callback::Callback(Context context, std::function<void()> callback) : context(std::move(context)) {
      handle = this->context.addCallback(std::move(callback));
   }

   Context::Callback::Callback(Callback&& other) noexcept : context(std::move(other.context)), handle(std::move(other.handle)), active(other.active) {
      other.active = false;
   }

   Context::Callback& Context::Callback::operator=(Callback&& other) noexcept {
      if (this != &other) {
         remove();
         context = std::move(other.context);
         handle = std::move(other.handle);
         active = other.active;
         other.active = false;
      }
      return *this;
   }

   void Context::Callback::remove() {
      if (active) {
         context.removeCallback(handle);
         active = false;
      }
   }

   Context::Callback::~Callback() { remove(); }
   /*~Context*/
} // namespace SN