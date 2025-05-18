#include <zmq.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cmath>

// Last sent position for dead-band filtering
double last_x = 0, last_y = 0, last_z = 0;
double threshold = 0.1; // Threshold for dead-band

int main() {
    // Subscriber connects to VM1
    zmq::context_t ctx(1);
    zmq::socket_t sub(ctx, zmq::socket_type::sub);
    sub.connect("tcp://vm1:5555");  // Connect to VM1 by container name
    sub.set(zmq::sockopt::subscribe, "");

    // Publisher for filtered data
    zmq::socket_t pub(ctx, zmq::socket_type::pub);
    pub.bind("tcp://*:5556");

    std::cout << "VM2 started - subscribing to vm1:5555, publishing on *:5556" << std::endl;

    while (true) {
        zmq::message_t msg;
        auto result = sub.recv(msg, zmq::recv_flags::none);
        
        // Parse the message
        std::string data = msg.to_string();
        std::istringstream iss(data);
        std::string ts_str;
        double x, y, z;
        char comma;
        
        // Format: "timestamp,x,y,z"
        std::getline(iss, ts_str, ',');
        iss >> x >> comma >> y >> comma >> z;
        
        // Apply dead-band filter
        bool should_send = (std::abs(x - last_x) > threshold) || 
                           (std::abs(y - last_y) > threshold) || 
                           (std::abs(z - last_z) > threshold);
        
        if (should_send) {
            // Update last sent position
            last_x = x;
            last_y = y;
            last_z = z;
            
            // Forward the timestamp
            pub.send(zmq::buffer(ts_str), zmq::send_flags::none);
            std::cout << "VM2 forwarded: " << ts_str << std::endl;
        }
    }
    
    return 0;
}