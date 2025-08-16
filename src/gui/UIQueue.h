#pragma once

#include <functional>
#include <queue>
#include <mutex>
#include <memory>

/**
 * @class UIQueue
 * @brief A thread-safe singleton queue for posting functions to be executed on the UI thread.
 *
 * This class provides a mechanism for background threads to safely schedule operations
 * (like GUI updates) on the main UI thread. Background threads can push std::function
 * objects onto the queue. The UI thread should periodically poll this queue and execute
 * the functions.
 */
class UIQueue {
public:
    /**
     * @brief Get the singleton instance of the UIQueue.
     */
    static UIQueue& getInstance() {
        static UIQueue instance;
        return instance;
    }

    /**
     * @brief Push a function onto the queue to be executed on the UI thread.
     * @param func The function to execute. This function must be callable with no arguments.
     */
    void push(std::function<void()> func) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(func));
    }

    /**
     * @brief Pop a function from the queue.
     * @param func A reference to a std::function that will be filled with the popped function.
     * @return True if a function was popped, false if the queue was empty.
     */
    bool pop(std::function<void()>& func) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) {
            return false;
        }
        func = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    /**
     * @brief Check if the queue is empty.
     * @return True if the queue is empty, false otherwise.
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

private:
    UIQueue() = default;
    ~UIQueue() = default;

    // Non-copyable and non-movable
    UIQueue(const UIQueue&) = delete;
    UIQueue& operator=(const UIQueue&) = delete;
    UIQueue(UIQueue&&) = delete;
    UIQueue& operator=(UIQueue&&) = delete;

    std::queue<std::function<void()>> m_queue;
    mutable std::mutex m_mutex;
};
