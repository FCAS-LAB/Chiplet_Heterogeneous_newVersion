#ifndef __COMMON_HPP__
#define __COMMON_HPP__
#include <mutex>
#include <queue>

namespace CCPS {
    class TimeSlice;

    TimeSlice& getTimeSlice();
    void resetSimTime();
    void run(const int time_in_ps);


    struct PacketInfo {
        int packet_index{0};
        int tag_index{0};
        int ecc{0};
        bool finished{false};
    };

    // Singleton
    class PacketInfoManager {
    public:
        static PacketInfoManager& GetInstance() {
            // C++11 guarantees thread-safe initialization of static local variables.
            static PacketInfoManager instance;
            return instance;
        }
        // Example member function
        void addPacket(PacketInfo packet_info);
        PacketInfo peek();
        void incTagIndex();
        void setFinished();
        PacketInfo popPacket();

        // Delete copy constructor and assignment operator to prevent cloning
        PacketInfoManager(const PacketInfoManager&) = delete;
        PacketInfoManager& operator=(const PacketInfoManager&) = delete;

    private:
        PacketInfoManager() {}
        ~PacketInfoManager() {}
        std::queue<PacketInfo> _packet_queue;
        std::mutex _mutex;
    }; // class PacketInfoManager
} // namespace CCPS

#endif // __COMMON_HPP__