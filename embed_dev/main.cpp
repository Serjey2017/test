#include <iostream>
#include <cppkafka/cppkafka.h>
#include <thread>
#include <memory>
#include <chrono>
#include <csignal>

using std::getline;
using std::cin;
using std::cout;
using std::endl;
using std::chrono::seconds;
using std::thread;

using namespace std;
using namespace cppkafka;

bool _do_work = 1;

void worker_broadcaster(Producer &prod, string key){

    const char* _bash_cmd = "cat /sys/class/thermal/thermal_zone0/temp";
    while (_do_work) {
        string message{};
        char buffer[8];
        FILE* pipe = popen(_bash_cmd, "r");
        if (!pipe) throw std::runtime_error("popen() failed!");
        try {
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                message += buffer;
            }
        } catch (...) {
            pclose(pipe);
            throw;
        }
        pclose(pipe);
        prod.produce(MessageBuilder("manual_topic").key(key).payload(message));
        std::this_thread::sleep_for(seconds(1));
    }
    prod.flush();
    cout << "thread " << this_thread::get_id() << " was stopped" << endl;
}

void worker_subscriber(Consumer &cons, ConsumerDispatcher &disp, string &name){

    while (_do_work) {
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
                    // Now commit the message
                    cons.commit(msg);
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
function<void()> on_signal;

void signal_handler(int) {
    on_signal();
}

int main() {
    cout << "This is embedded device part; periodically sends cpu temp, and manual msgs by desire \n" <<
         " also receive service msgs" << endl;
    Configuration config = {
            {"metadata.broker.list", "127.0.0.1:9092"}
    };

    Configuration config_service = {
            {"metadata.broker.list", "127.0.0.1:9092"}
    };

    TopicConfiguration default_topic_config;

    const auto callback = [](const Topic &, const Buffer &key, int32_t partition_count) {
        return stoi(key) % partition_count;
    };

    default_topic_config.set_partitioner_callback(callback);
    config.set_default_topic_configuration(move(default_topic_config));

    Producer manual_producer(config);
    Producer timed_producer(config);

    Consumer consumer(config_service);
    consumer.set_assignment_callback([](const TopicPartitionList &partitions) {
        cout << "Got assigned: " << partitions << endl;
    });

    consumer.set_revocation_callback([](const TopicPartitionList &partitions) {
        cout << "Got revoked: " << partitions << endl;
    });

    ConsumerDispatcher dispatcher(consumer);

    on_signal = [&]() {
        dispatcher.stop();
        _do_work = 0;
        manual_producer.flush();
    };
    signal(SIGINT, signal_handler);


    const string _key1 = "42";
    const string _key2 = "43";

    string service_name ="service_msg";

    thread automatic_spam(worker_broadcaster, std::ref(timed_producer), std::ref(_key2));
    thread service_receiver(worker_subscriber, std::ref(consumer), std::ref(dispatcher), std::ref(service_name));
    automatic_spam.detach();
    service_receiver.detach();

    cout << "thread start in " << automatic_spam.get_id() << endl;

    string line;
    while (getline(cin, line)) {
        manual_producer.produce(MessageBuilder("manual_topic").key(_key1).payload(line));
    }
    _do_work = 0;
    manual_producer.flush();
    exit(0);
}