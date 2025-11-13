#ifndef GG_IPC_SUBSCRIPTION_HPP
#define GG_IPC_SUBSCRIPTION_HPP

#include <gg/types.hpp>
#include <cstddef>
#include <cstdint>
#include <compare>
#include <functional>
#include <utility>

extern "C" {
void ggipc_close_subscription(GgIpcSubscriptionHandle handle) noexcept;
}

namespace gg::ipc {

/// Subscription handle with std::unique_ptr semantics. The underlying
/// handle has two copies; one is returned by a Client subscription method, and
/// another is passed to SubscriptionCallbacks, which are called by a
/// library-controlled thread. Either copy of the handle can terminate the
/// underlying subscription.
class [[nodiscard]] Subscription {
private:
    GgIpcSubscriptionHandle handle {};

public:
    /// A default-constructed Subscription is guaranteed to refer to no
    /// subscription at all.
    constexpr Subscription() noexcept = default;

    explicit constexpr Subscription(
        GgIpcSubscriptionHandle subscription_handle
    ) noexcept
        : handle { subscription_handle } {
    }

    /// Moved Subscriptions are guaranteed afterwards to refer to no
    /// subscription.
    constexpr Subscription(Subscription &&subscription) noexcept
        : handle { subscription.release() } {
    }

    Subscription &operator=(Subscription &&subscription) noexcept {
        // Guard against self-moves
        if (subscription != *this) {
            reset(subscription.release());
        }
        return *this;
    }

    Subscription &operator=(const Subscription &) = delete;
    Subscription(const Subscription &) = delete;

    ~Subscription() noexcept {
        close();
    }

    /// Returns true if Subscription may refer to an active subscription.
    /// Only semantically meaningful to detect default/moved values.
    /// Does not detect whether another thread calling close() on its copy of
    /// this handle (i.e. calling close() on a SubscriptionCallback's
    /// Subscription& parameter).
    constexpr bool holds_subscription() const noexcept {
        return handle.val != 0;
    }

    /// Copies the current raw handle. Use only for hashing.
    constexpr GgIpcSubscriptionHandle get() const noexcept {
        return handle;
    }

    /// Relinquish ownership of handle without closing it.
    [[nodiscard]]
    constexpr GgIpcSubscriptionHandle release() noexcept {
        return std::exchange(handle, GgIpcSubscriptionHandle {});
    }

    /// closes current subscription and replaces it with a new handle.
    void reset(GgIpcSubscriptionHandle new_handle) noexcept {
        close();
        handle = new_handle;
    }

    /// Swap handles
    constexpr void swap(Subscription &other) noexcept {
        std::swap(handle, other.handle);
    }

    /// Non-member swap
    friend constexpr void swap(Subscription &lhs, Subscription &rhs) noexcept {
        lhs.swap(rhs);
    }

    /// Same as holds_subscription()
    constexpr operator bool() const noexcept {
        return holds_subscription();
    }

    /// Sends a terminate stream request to the IPC server. After this function
    /// returns, the Subscription's associated SubscriptionCallback (if any) is
    /// no longer called for messages to this Subscription after this method
    /// returns. Threadsafe and reentrant. Valid even if Subscription is moved /
    /// default-constructed. Valid to call from within a SubscriptionCallback
    /// handler, but such a call is not visible to other threads until they call
    /// close as well.
    void close() noexcept {
        // This check avoids locking a mutex when Subscription is
        // default-initialized
        if (holds_subscription()) {
            ggipc_close_subscription(release());
        }
    }

    /// Comparisons for associative containers.
    constexpr std::strong_ordering operator<=>(
        const Subscription &other
    ) const noexcept {
        return handle.val <=> other.handle.val;
    }
};

}

/// Subscription hashing
template <> class std::hash<gg::ipc::Subscription> {
public:
    std::size_t operator()(
        const gg::ipc::Subscription &subscription
    ) const noexcept {
        return std::hash<std::uint32_t> {}(subscription.get().val);
    }
};

#endif
