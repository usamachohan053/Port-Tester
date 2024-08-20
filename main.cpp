#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <windows.h>
#include <cstdint>
#include <limits> // For std::numeric_limits

// Function to list all available COM ports
std::vector<std::string> list_com_ports() {
    std::vector<std::string> com_ports;
    char com_name[10];
    
    for (int i = 1; i <= 256; ++i) {
        sprintf_s(com_name, "COM%d", i);
        HANDLE com_handle = CreateFileA(com_name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        
        if (com_handle != INVALID_HANDLE_VALUE) {
            com_ports.push_back(com_name);
            CloseHandle(com_handle);
        }
    }
    return com_ports;
}

// Function to configure the selected COM port 010300000002C40B
HANDLE configure_com_port(const std::string& port_name, DWORD baud_rate) {
    HANDLE hComm = CreateFileA(port_name.c_str(),
                               GENERIC_READ | GENERIC_WRITE,
                               0,
                               nullptr,
                               OPEN_EXISTING,
                               0,
                               nullptr);

    if (hComm == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening COM port " << port_name << std::endl;
        return nullptr;
    }

    // Configure the serial port parameters
    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hComm, &dcbSerialParams)) {
        std::cerr << "Error getting COM port state" << std::endl;
        CloseHandle(hComm);
        return nullptr;
    }

    dcbSerialParams.BaudRate = baud_rate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hComm, &dcbSerialParams)) {
        std::cerr << "Error setting COM port state" << std::endl;
        CloseHandle(hComm);
        return nullptr;
    }

    return hComm;
}

// Function to convert a hexadecimal string to a vector of uint8_t
std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_string = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(strtol(byte_string.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

// Function to send the hexadecimal signal via the selected COM port
void send_signal(HANDLE hComm, const std::vector<uint8_t>& signal) {
    DWORD bytes_written;

    if (!WriteFile(hComm, signal.data(), signal.size(), &bytes_written, nullptr)) {
        std::cerr << "Error writing to COM port" << std::endl;
    } else {
        std::cout << "Successfully sent " << bytes_written << " bytes." << std::endl;
    }
}

// Function to read the reply from the COM port with timeout and retry
std::vector<uint8_t> read_reply(HANDLE hComm, DWORD timeout_ms) {
    std::vector<uint8_t> buffer(1024);
    DWORD bytes_read;
    bool reply_received = false;

    // Setup timeout for reading
    // COMMTIMEOUTS timeouts = { 0 };
    // timeouts.ReadIntervalTimeout = 50;
    // timeouts.ReadTotalTimeoutConstant = timeout_ms;
    // timeouts.ReadTotalTimeoutMultiplier = 0;
    // SetCommTimeouts(hComm, &timeouts);

    // Wait for data to arrive
    DWORD event_mask;
    // if (WaitCommEvent(hComm, &event_mask, nullptr)) {
        if (!ReadFile(hComm, buffer.data(), buffer.size(), &bytes_read, nullptr)) {
            std::cerr << "Error reading from COM port" << std::endl;
        } else {
            buffer.resize(bytes_read); // Resize the buffer to the actual number of bytes read
            std::cout << "Successfully received " << bytes_read << " bytes." << std::endl;
            reply_received = true;
        }
    // } else {
    //     std::cerr << "Timeout or error waiting for data" << std::endl;
    // }

    if (!reply_received) {
        buffer.clear();
    }
    
    return buffer;
}

// Function to print the reply in hexadecimal format
void print_hex(const std::vector<uint8_t>& data) {
    std::cout << "Reply in hex: ";
    for (const auto& byte : data) {
        printf("%02X ", byte);
    }
    std::cout << std::endl;
}

int main() {
    std::vector<std::string> com_ports = list_com_ports();

    // Display the COM ports as a numeric list
    for (size_t i = 0; i < com_ports.size(); ++i) {
        std::cout << i + 1 << ") " << com_ports[i] << std::endl;
    }

    // Ask the user to select a COM port
    int selection;
    std::cout << "Select a COM port by number: ";
    std::cin >> selection;

    if (selection > 0 && selection <= static_cast<int>(com_ports.size())) {
        std::string selected_port = com_ports[selection - 1];
        std::cout << "You selected: " << selected_port << std::endl;

        // Ask the user to select a baud rate
        int baud_rate;
        std::cout << "Enter the baud rate (e.g., 9600, 19200, 115200): ";
        std::cin >> baud_rate;

        HANDLE hComm = configure_com_port(selected_port, baud_rate);
        if (hComm == nullptr) {
            return 1;
        }

        // Set timeout
        // std::cout << "Enter the timeout value in milliseconds (e.g., 5000): ";
        DWORD timeout_ms = 5000;
        // while (true) {
        //     std::cin >> timeout_ms;

        //     // Check if the input is valid
        //     if (std::cin.fail() || timeout_ms <= 0) {
        //         std::cin.clear(); // Clear the error flag
        //         std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard invalid input
        //         std::cerr << "Invalid input. Please enter a positive integer value." << std::endl;
        //         std::cout << "Enter the timeout value in milliseconds (e.g., 5000): ";
        //     } else {
        //         break; // Valid input
        //     }
        // }


        bool continue_sending = true;

        while (continue_sending) {
            // Ask the user to input the signal in hexadecimal form
            std::string hex_signal;
            std::cout << "Enter the signal in hexadecimal form (e.g., A5FF00): ";
            std::cin >> hex_signal;

            // Convert the hexadecimal string to a vector of bytes
            std::vector<uint8_t> signal = hex_to_bytes(hex_signal);

            // Send the signal
            send_signal(hComm, signal);

            // Wait and read the reply
            std::vector<uint8_t> reply = read_reply(hComm, timeout_ms);

            if (!reply.empty()) {
                // Print the reply in hexadecimal form
                print_hex(reply);
                //continue_sending = false;
            } else {
                std::cout << "No reply received.\n";
            }

            // Retry
            std::cout << "Do you want to send a new message? (y/n): ";
            char retry;
            std::cin >> retry;
            if (retry != 'y' && retry != 'Y') {
                continue_sending = false;
            }
        }

        // Close the COM port
        CloseHandle(hComm);
    } else {
        std::cout << "Invalid selection!" << std::endl;
    }

    // Wait for the user to press Enter before exiting
    std::cout << "Press Enter to exit...";
    std::cin.ignore();
    std::cin.get();

    return 0;
}
