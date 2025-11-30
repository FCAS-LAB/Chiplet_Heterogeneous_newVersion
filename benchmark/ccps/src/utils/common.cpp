#include "utils/common.hpp"
#include "utils/time_slice.hpp"
#include "utils/print.hpp"

namespace CCPS {
    TimeSlice& getTimeSlice() {
        ENTER_FUNC();
        return TimeSlice::getInstance();
    }
    void resetSimTime() {
        ENTER_FUNC();
        getTimeSlice().resetTime();
    }
    void run(const int time_in_ps) {
        ENTER_FUNC();
        //std::cout << "=========== step ================" << std::endl;
        getTimeSlice().run(time_in_ps);
    }

    void PacketInfoManager::addPacket(PacketInfo packet_info) {
        _mutex.lock();
        _packet_queue.push(packet_info);
        _mutex.unlock();
    }
    PacketInfo PacketInfoManager::peek() {
        PacketInfo packet_info;
        _mutex.lock();
        packet_info = _packet_queue.front();
        _mutex.unlock();
        return packet_info;
    }

    void PacketInfoManager::incTagIndex() {
        _mutex.lock();
        _packet_queue.front().tag_index++;
        _mutex.unlock();
    }
    void PacketInfoManager::setFinished() {
        _mutex.lock();
        _packet_queue.front().finished = true;
        _mutex.unlock();
    }
    PacketInfo PacketInfoManager::popPacket() {
        PacketInfo packet_info;
        _mutex.lock();
        packet_info = _packet_queue.front();
        _packet_queue.pop();
        _mutex.unlock();
        return packet_info;
    }

} // namespace CCPS