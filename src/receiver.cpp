#include <iostream>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include "FooTypeSupportC.h"
#include "FooTypeSupportImpl.h"
#include "dds/DCPS/WaitSet.h"
#include <atomic>

std::atomic<bool> running{true};

class DataReaderListenerImpl : public DDS::DataReaderListener {
    void on_requested_deadline_missed(DDS::DataReader_ptr, const DDS::RequestedDeadlineMissedStatus&) override {
        std::cout << "on_requested_deadline_missed" << std::endl;
    }
    void on_requested_incompatible_qos(DDS::DataReader_ptr, const DDS::RequestedIncompatibleQosStatus&) override {
        std::cout << "on_requested_incompatible_qos" << std::endl;
    }
    void on_sample_rejected(DDS::DataReader_ptr, const DDS::SampleRejectedStatus&) override {
        std::cout << "on_sample_rejected" << std::endl;
    }
    void on_liveliness_changed(DDS::DataReader_ptr, const DDS::LivelinessChangedStatus& status) override {
        std::cout << "on_liveliness_changed: alive=" << status.alive_count
                  << " not_alive=" << status.not_alive_count << std::endl;
    }

    void on_data_available(DDS::DataReader_ptr reader) override {
        Try::FooDataReader_var reader_i = Try::FooDataReader::_narrow(reader);
        if (!reader_i) {
            std::cerr << "read: _narrow failed." << std::endl;
            return;
        }

        Try::Foo message;
        DDS::SampleInfo si;
        DDS::ReturnCode_t status = reader_i->take_next_sample(message, si);

        if (status == DDS::RETCODE_OK) {
            if (si.valid_data) {
                std::cout << "Message: bazz=" << message.bazz.in()
                    << "  temp=" << message.temp
                    << "  val[0]=" << message.values[0]
                    << "  dynamic[0]=" << message.dynamic[0]
                    << "  fav=" << message.fav << std::endl;
            } else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
                std::cout << "instance is disposed" << std::endl;
            } else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
                std::cout << "instance is unregistered" << std::endl;
            } else {
                std::cerr << "ERROR: received unknown instance state "
                    << si.instance_state << std::endl;
            }
        } else if (status == DDS::RETCODE_NO_DATA) {
            std::cerr << "ERROR: reader received DDS::RETCODE_NO_DATA!" << std::endl;
        } else {
            std::cerr << "ERROR: read Message: Error: " << status << std::endl;
        }
    }

    void on_subscription_matched(DDS::DataReader_ptr, const DDS::SubscriptionMatchedStatus& status) override {
        if (status.current_count > 0) {
            std::cout << "Publisher connected (total matched: " << status.total_count << ")" << std::endl;
        } else {
            std::cout << "Publisher disconnected — will reconnect automatically when it returns" << std::endl;
        }
    }
    void on_sample_lost(DDS::DataReader_ptr, const DDS::SampleLostStatus& status) override {
        std::cout << "on_sample_lost: total=" << status.total_count << std::endl;
    }
};

int main() {
    char* arg1 = const_cast<ACE_TCHAR*>("opendds-try");
    char* arg2 = const_cast<ACE_TCHAR*>("-DCPSConfigFile");
    char* arg3 = const_cast<ACE_TCHAR*>("../rtps.ini");

    std::array<ACE_TCHAR*, 6> args = {arg1, arg2, arg3, nullptr, nullptr, nullptr};
    int args_count = 3;

    DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(args_count, args.data());

    auto participant = dpf->create_participant(
        0, PARTICIPANT_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!participant) { std::cerr << "create_participant failed.\n"; return 1; }

    Try::FooTypeSupport_var mts = new Try::FooTypeSupportImpl;
    if (DDS::RETCODE_OK != mts->register_type(participant, "")) {
        std::cerr << "register_type failed.\n"; return 1;
    }

    auto type_name = mts->get_type_name();
    auto topic = participant->create_topic(
        "Foo", type_name, TOPIC_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!topic) { std::cerr << "create_topic failed.\n"; return 1; }

    auto subscriber = participant->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!subscriber) { std::cerr << "create_subscriber failed.\n"; return 1; }

    DDS::DataReaderQos reader_qos;
    subscriber->get_default_datareader_qos(reader_qos);
    reader_qos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
    reader_qos.reliability.max_blocking_time.nanosec = 10e6;
    reader_qos.deadline.period.nanosec = 10e6;
    reader_qos.latency_budget.duration.nanosec = 10e6;

    DDS::DataReaderListener_var listener(new DataReaderListenerImpl);
    auto reader = subscriber->create_datareader(
        topic, reader_qos, listener, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!reader) { std::cerr << "create_datareader failed.\n"; return 1; }

    DDS::StatusCondition_var condition = reader->get_statuscondition();
    condition->set_enabled_statuses(
        DDS::SUBSCRIPTION_MATCHED_STATUS | DDS::DATA_AVAILABLE_STATUS
    );

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    std::cout << "Receiver ready — waiting for publisher..." << std::endl;

    while (running) {
        DDS::ConditionSeq conditions;
        DDS::Duration_t timeout = { 5, 0 };

        auto rc = ws->wait(conditions, timeout);

        if (rc == DDS::RETCODE_TIMEOUT) {
            DDS::SubscriptionMatchedStatus matches;
            reader->get_subscription_matched_status(matches);
            std::cout << "[heartbeat] publishers matched: " << matches.current_count << std::endl;
            continue;
        }

        if (rc != DDS::RETCODE_OK) {
            std::cerr << "wait() failed: " << rc << std::endl;
            continue;
        }
    }

    ws->detach_condition(condition);
    participant->delete_contained_entities();
    dpf->delete_participant(participant);
    TheServiceParticipant->shutdown();

    return 0;
}
