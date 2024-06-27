//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include "server.h"
#include "config_parser.h"
#include "logger.h"

using boost::asio::ip::tcp;

const int NUM_THREADS = 4;

int main(int argc, char* argv[]) {
  try {
    Logger *logger = Logger::get_global_log();
    if (argc != 2) {
      std::cerr << "Usage: ./server <path to config file>\n";
      logger->logError("Usage: ./server <path to config file>\n");
      return 1;
    }

    NginxConfigParser config_parser;
    NginxConfig config;
    if (!config_parser.Parse(argv[1], &config)) {
      std::cerr << "Improperly formatted config file.\n";
      logger->logError("Improperly formatted config file.\n");
      return 1;
    }

    std::string port = config.FindPortNumber();
    if (port.empty()) {
      std::cerr << "Please provide a port number" << std::endl;
      logger->logError("Please provide a port number\n");
      return 1;
    }

    std::vector<HandlerConfig> handlers = config.GetRequestHandlers();
    if (handlers.empty()) {
      std::cerr << "Please provide at least one handler, and ensure serving locations are unique" << std::endl;
      logger->logError("Please provide at least one handler and ensure serving locations are unique\n");
      return 1;
    }


    boost::asio::io_service io_service;
    server s(io_service, std::stoi(port), handlers);

    boost::asio::signal_set signals(io_service, SIGTERM, SIGINT);
  
    // Wait for a signal to occur
    signals.async_wait([&io_service, logger](const boost::system::error_code& error, int signal_number) {
      if (!error) {
          std::cout << "Shutting down server" << std::endl;
          logger->logWarning("Shutting down server\n");
          io_service.stop();
        }
      });
    
    logger->logInfo("Starting server on port " + port + "\n");
    
    // Create a pool of threads to run the io_service
    boost::thread_group threads;
    for (std::size_t i = 0; i < NUM_THREADS; i++) {
      threads.create_thread([&io_service](){
        io_service.run();
      });
    }

    // Wait for all threads in the pool to exit
    threads.join_all();


  } catch (std::exception& e) {
    Logger* logger = Logger::get_global_log();
    logger->logError("Exception: " + std::string(e.what()) + "\n");
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
