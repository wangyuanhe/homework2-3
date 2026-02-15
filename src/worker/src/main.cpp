#include <iostream>
#include <thread>
#include <memory>
#include <bits/stdc++.h>
#include <cstdlib>
#include <ctime>

#include "rclcpp/rclcpp.hpp"
#include "rm_server/msg/get_el_gamal_params.hpp"
#include "rm_server/srv/el_gamal_encrypt.hpp"
#include "std_msgs/msg/int64.hpp"

#define uint64 unsigned long long

class Homework : public rclcpp :: Node{
private:
    uint64 memp;
    uint64 memn;

    rclcpp::Subscription<rm_server::msg::GetElGamalParams>::SharedPtr my_subscribe;
    rclcpp::Client<rm_server::srv::ElGamalEncrypt>::SharedPtr my_client;
    rclcpp::Publisher<std_msgs::msg::Int64>::SharedPtr my_publisher;
    uint64 qpow(uint64 a,uint64 b,uint64 mod){
        uint64 sum = 1;
        while(b){
            if(b & 1)sum = sum * a % mod;
            a = a * a % mod;
            b >>= 1;
        }
        return sum;
    } 
    uint64 exgcd(uint64 a,uint64 b){
        if(a == 1)return 1;
        return exgcd(b % a,b) * (b - (b / a)) % b;
    }
    
    void send_request(uint64 b){
        RCLCPP_INFO(this->get_logger(),"send public key:%lld",b);
        if (!my_client->wait_for_service(std::chrono::seconds(1))) {
            RCLCPP_WARN(this->get_logger(), "Waiting for service...");
            return;
        }

        auto request = std::make_shared<rm_server::srv::ElGamalEncrypt::Request>();
        request->public_key = b;

        my_client->async_send_request(
            request,std::bind(&Homework::my_callback2,this,std::placeholders::_1));
    }
public:

    Homework():Node("worker"){

        my_subscribe = this->create_subscription<rm_server::msg::GetElGamalParams>
        ("/elgamal_params",10,bind(&Homework::my_callback,this,std::placeholders::_1));

        my_publisher = this->create_publisher<std_msgs::msg::Int64>
        ("/elgamal_result", 10);

        my_client = this->create_client<rm_server::srv::ElGamalEncrypt>("/elgamal_service");
    }
private:
    inline void my_callback(const rm_server::msg::GetElGamalParams::SharedPtr msg){
        RCLCPP_INFO(this->get_logger(), "I heard: '%ld %ld'", msg->p,msg->a);

        memp = msg->p;

        srand(static_cast<unsigned int>(time(0)));
        unsigned int n = rand() % 100;

        memn = n;

        send_request(qpow(msg->a,n,msg->p));
    }
    inline void my_callback2(rclcpp::Client<rm_server::srv::ElGamalEncrypt>::SharedFuture result_future){
        auto response = result_future.get();
        RCLCPP_INFO(this->get_logger(), "I heard(2): '%ld %ld'",response->y1,response->y2);

        uint64 p1 = qpow(response->y1,memn,memp);
        uint64 p2 = exgcd(p1,memp);
        std_msgs::msg::Int64 b;
        b.data = (response->y2 * p2) % memp;
        my_publisher->publish(b);
    }
};

int main(int argc, char **argv) {
    
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Homework>());
    rclcpp::shutdown();
    return 0;
}