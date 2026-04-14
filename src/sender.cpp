#include <iostream>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include "FooTypeSupportC.h"
#include "FooTypeSupportImpl.h"
#include "thread"

std::atomic<bool> running{true};

int main() {
    char* arg1 = const_cast<ACE_TCHAR*>("opendds-try");
    char* arg2 = const_cast<ACE_TCHAR*>("-DCPSConfigFile");
    char* arg3 = const_cast<ACE_TCHAR*>("../rtps.ini");

    std::array<ACE_TCHAR*, 6> args = {arg1, arg2, arg3, nullptr, nullptr, nullptr};
    int args_count = 3;

    DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(args_count, args.data());

    auto participant = dpf->create_participant(
            0,
            PARTICIPANT_QOS_DEFAULT,
            0,
            OpenDDS::DCPS::DEFAULT_STATUS_MASK
            );
    if (!participant) {
        std::cerr << "create_participant failed." << std::endl;
        return 1;
    }

    Try::FooTypeSupport_var mts = new Try::FooTypeSupportImpl();
    if (DDS::RETCODE_OK != mts->register_type(participant, "")) {
        std::cerr << "register_type failed." << std::endl;
        return 1;
    }

    auto type_name= mts->get_type_name();
    auto topic = participant->create_topic("Foo", type_name, TOPIC_QOS_DEFAULT, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!topic) {
        std::cerr << "create_topic failed." << std::endl;
        return 1;
    }

    auto publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!publisher) {
        std::cerr << "create_publisher failed." << std::endl;
        return 1;
    }

    DDS::DataWriterQos writer_qos;
    publisher->get_default_datawriter_qos(writer_qos);
    writer_qos.writer_data_lifecycle.autodispose_unregistered_instances = false;
    writer_qos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
    writer_qos.reliability.max_blocking_time.nanosec = 10e6;
    writer_qos.deadline.period.nanosec = 10e6;
    writer_qos.latency_budget.duration.nanosec = 10e6;

    auto writer = publisher->create_datawriter(topic, writer_qos, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!writer) {
        std::cerr << "create_datawriter failed." << std::endl;
        return 1;
    }

    Try::FooDataWriter_var message_writer = Try::FooDataWriter::_narrow(writer);
    Try::Foo message{};
    message.bazz = "ABC";
    message.temp = 0.0;
    message.values[0] = 69;
    message.values[1] = 433;
    message.values[2] = 1028;
    message.values[3] = 212;
    message.values[4] = 33;

    // VERY IMPORTANT !
    message.dynamic.length(3);
    message.dynamic[0] = 6969;

    message.fav = Try::GREEN;

    while (running) {
        message.temp += 0.1;
        auto err = message_writer->write(message, DDS::HANDLE_NIL);
        if (err != DDS::RETCODE_OK) {
            std::cerr << "Something went wrong" << std::endl;
            return 1;
        } else {
            std::cout << "Sent !" << std::endl;
        }

        if (false) {
            DDS::Duration_t timeout = { 30, 0 };
            if (message_writer->wait_for_acknowledgments(timeout) != DDS::RETCODE_OK) {
                std::cerr <<  "wait_for_acknowledgments failed!\n" << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    participant->delete_contained_entities();
    dpf->delete_participant(participant);
    TheServiceParticipant->shutdown();

    return 0;
}
