#include <Poco/SharedMemory.h>
#include <Poco/Exception.h>
#include <iostream>
#include <cstring>

int test() {
  try {
    const std::string shmName = "test_shm";
    const size_t size = 27;
    Poco::SharedMemory shm2(shmName, size, Poco::SharedMemory::AM_READ, nullptr, false);
    std::cout << "Data read from shared memory: " << shm2.begin() << std::endl;
  }
  catch (const Poco::Exception& e) {
    std::cerr << "POCO Exception: " << e.displayText() << std::endl;
    return 1;
  }
  catch (const std::exception& e) {
    std::cerr << "Standard Exception: " << e.what() << std::endl;
    return 1;
  }
  return 0;

}
int main() {
    try {
        // Define shared memory name and size
        const std::string shmName = "test_shm";
        // const size_t size = 4096;
        const size_t size = 27;
        // Create or open shared memory
        Poco::SharedMemory shm(shmName, size, Poco::SharedMemory::AM_WRITE, nullptr, true);

        // Write data to shared memory
        const char* message = "Hello, POCO shared memory!";
        std::memcpy(shm.begin(), message, std::strlen(message) + 1); // Include null terminator

        // Read data from shared memory
        // char* readData = shm.begin();
        // std::cout << "Data read from shared memory: " << readData << std::endl;


        // POCO automatically unmaps memory when 'shm' goes out of scope
        // Unlink the shared memory object
        // Poco::SharedMemory::unlink(shmName);
    }
    catch (const Poco::Exception& e) {
        std::cerr << "POCO Exception: " << e.displayText() << std::endl;
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Standard Exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
    test();
    return 0;
}
