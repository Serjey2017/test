#include <iostream>
#include <cppkafka/cppkafka.h>
#include <thread>
#include <memory>
#include <chrono>
#include <csignal>
#include <functional>
#include <string>
#include "boost/lexical_cast.hpp"
#include <queue>

using namespace std;
using namespace cppkafka;
using std::chrono::seconds;

bool _do_work = 1;
bool _busy_status = 0;

std::vector<double> interp_buffer;

double calc_interpolation(double val){
    _busy_status = 1;
    const int max_elem_num = 3;
    interp_buffer.insert(interp_buffer.begin(), val);
    if(interp_buffer.size() > max_elem_num)
        interp_buffer.pop_back(); // keep only 3 values

        double sum{0.0};
        for (auto tmp_val:interp_buffer){
            sum += tmp_val;
        }
    _busy_status = 0;
    return (sum/interp_buffer.size());
}

void worker_receiver_interpolation(Consumer &cons, ConsumerDispatcher &disp, string &name, void* func){
    while(_do_work) {
        try {
            cons.subscribe({name});
        }
        catch (...) {
            cout << "problem to subscribe" << endl;
            std::this_thread::sleep_for(seconds(1));
            continue;
        }

        disp.run(
                // Callback executed whenever a new message is consumed
                [&](Message msg) {
                    // Print the key (if any)
                    if (msg.get_key()) {
                        cout << msg.get_key() << " -> ";
                    }
                    // Print the payload
                    cout << msg.get_payload() << endl;
                    try {
                        double val = boost::lexical_cast<double>(msg.get_payload());
                        cout << "interpolated value would be = " << calc_interpolation(val);
                    }
                    catch(...) { cout << "not a number" << endl; }
                },
                // Whenever there's an error (other than the EOF soft error)
                [](Error error) {
                    cout << "[+] Received error notification: " << error << endl;
                },
                // Whenever EOF is reached on a partition, print this
                [](ConsumerDispatcher::EndOfFile, const TopicPartition &topic_partition) {
                    cout << "Reached EOF on partition " << topic_partition << endl;
                }
        );

    }
}


std::function<void()> on_signal;

void signal_handler(int) {
    on_signal();
}

function<void()> send_status;

int main() {
    std::cout << "This is server side app, doing interpolation in some cases" << std::endl;

    std::string async_topic_name ="manual_topic";

    Configuration config = {
            { "metadata.broker.list", "127.0.0.1:9092" }
    };

    Configuration config_broadcaster = {
            { "metadata.broker.list", "127.0.0.1:9092" }
    };

    Consumer consumer(config);
    consumer.set_assignment_callback([](const TopicPartitionList &partitions) {
        cout << "Got assigned: " << partitions << endl;
    });

    consumer.set_revocation_callback([](const TopicPartitionList &partitions) {
        cout << "Got revoked: " << partitions << endl;
    });

    ConsumerDispatcher dispatcher(consumer);

    on_signal = [&]() {
        dispatcher.stop();
        return 0;
    };

    Producer status_producer(config_broadcaster);

    signal(SIGINT, signal_handler);

    thread manual_interp(worker_receiver_interpolation, std::ref(consumer), std::ref(dispatcher),
                         std::ref(async_topic_name), *send_status);
    manual_interp.detach();

    return 0;
}
