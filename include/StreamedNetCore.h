#ifndef NCORE_STREAMED_NET_CORE_H
#define NCORE_STREAMED_NET_CORE_H

#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

#include "AsioInclude.h"

namespace SN {
   enum class NetworkMode {
      TCP,
      UDP
   };

   enum State {
      Offline,
      Created,
      Online,
      Reading,
      Writing,
      Accepting,
   };

   enum class Event {
      OnStart,
      Aborted,
      Connected,
      Resolved,
      DataSent,
      DataReceived,
      Disconnected
   };

   enum class Error {
      AlreadyStarted,
      AlreadyResolved,
      AlreadyConnected,
      ConnectFailed,
      ResolveFailed,
      AcceptFailed,
      ConnectionClosed,
      Aborted,
      WriteFailed,
      ReadFailed,
      AbortShutdownFailed,
      AbortCloseFailed,
      AcceptorAbortCancelFailed,
      AcceptorAbortCloseFailed,
      InvalidAddress
   };

   template <NetworkMode Mode>
   class Client;

   template <NetworkMode Mode>
   class Server;

   template <NetworkMode Mode>
   class Connection;

   using TCPClient = Client<NetworkMode::TCP>;
   using UDPClient = Client<NetworkMode::UDP>;

   using TCPServer = Server<NetworkMode::TCP>;
   using UDPServer = Server<NetworkMode::UDP>;

   using TCPConnection = Connection<NetworkMode::TCP>;
   using UDPConnection = Connection<NetworkMode::UDP>;

   class Context {
   private:
      using CallbackPtr = std::shared_ptr<std::function<void()>>;

      struct State {
         asio::io_context io;
         std::atomic_uint32_t usageCount{0};
         std::vector<CallbackPtr> callbacks;
         std::mutex callbackMutex;
      };

   public:
      using CallbackHandle = std::weak_ptr<std::function<void()>>;
      Context();

      void use();
      void release();
      size_t usage() const;

      CallbackHandle addCallback(std::function<void()> callback);
      void removeCallback(const CallbackHandle& callback);

      void poll();

      class UsageGuard;
      class Callback;

   private:
      std::shared_ptr<State> state;

   public:
      friend class Resolver;
      template <typename T>
      friend class WriteManager;
      template <NetworkMode Mode>
      friend class Client;
      template <NetworkMode Mode>
      friend class Server;
      template <NetworkMode Mode>
      friend class Connection;
   };

   class Context::UsageGuard {
   public:
      explicit UsageGuard(Context context);

      ~UsageGuard();

      UsageGuard(const UsageGuard&) = delete;
      UsageGuard& operator=(const UsageGuard&) = delete;

      UsageGuard(UsageGuard&& other) noexcept;

      UsageGuard& operator=(UsageGuard&& other) noexcept;

      void release();

   private:
      Context context;
      bool active = true;
   };

   class Context::Callback {
   public:
      Callback(Context context, std::function<void()> callback);

      ~Callback();

      Callback(const Callback&) = delete;
      Callback& operator=(const Callback&) = delete;

      Callback(Callback&& other) noexcept;

      Callback& operator=(Callback&& other) noexcept;

      void remove();

   private:
      Context context;
      CallbackHandle handle;
      bool active = true;
   };

   template <class Owner>
   class TickManager {
   public:
      void startTick() {
         auto& owner = static_cast<Owner&>(*this);
         auto state = owner.state;
         if (!state || state->callback) return;
         state->callback.emplace(Context::Callback(owner.context, [st = state]() {
            if (!st) return;

            std::lock_guard guard(st->mutex);
            if (!st->reference) return;
            st->reference->onTick();
         }));
      }

      void stopTick() {
         auto& owner = static_cast<Owner&>(*this);
         auto state = owner.state;
         if (!state || !state->callback) return;
         state->callback.reset();
      }

      virtual void onTick() {}
   };

   template <class Owner>
   class WriteManager {
   public:
      void startWrite() {
         auto& owner = static_cast<Owner&>(*this);
         auto state = owner.state;
         if (!state) return;

         asio::post(owner.context.state->io, [st = state]() {
            if (!st) return;
            std::lock_guard guard(st->mutex);
            if (!st->reference || st->writing) return;
            st->writing = true;
            st->reference->doWrite();
         });
      }

      void stopWrite() {
         auto& owner = static_cast<Owner&>(*this);
         auto state = owner.state;
         if (!state) return;

         asio::post(owner.context.state->io, [st = state]() {
            if (!st) return;
            st->writing = false;
         });
      }

      virtual void onWrite() {}
   };

   template <class Owner>
   class ReadManager {};

   class NetworkBase {
   public:
      virtual void disconnect() { std::cout << "no override\n"; }
      virtual void send(const std::vector<uint8_t>& msg) { std::cout << "no override\n"; }
      virtual int getState(State state) const { 
         std::cout << "no override\n"; 
         return 0;
      }
   };

   class BaseClient : public NetworkBase {
   public:
   };

   class BaseServer : public NetworkBase {
   public:
   };

   class BaseConnection : public NetworkBase {
   public:
   };
} // namespace SN

#endif // ~NCORE_STREAMED_NET_CORE_H