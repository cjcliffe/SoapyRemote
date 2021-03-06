// Copyright (c) 2015-2015 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "SoapyServer.hpp"
#include "SoapyRemoteDefs.hpp"
#include "SoapyURLUtils.hpp"
#include "SoapyRPCSocket.hpp"
#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <getopt.h>
#include <csignal>

/***********************************************************************
 * Print help message
 **********************************************************************/
static int printHelp(void)
{
    std::cout << "Usage SoapySDRServer [options]" << std::endl;
    std::cout << "  Options summary:" << std::endl;
    std::cout << "    --help \t\t\t\t Print this help message" << std::endl;
    std::cout << "    --bind \t\t\t\t Bind and serve forever" << std::endl;
    std::cout << std::endl;
    return EXIT_SUCCESS;
}

/***********************************************************************
 * Signal handler for Ctrl + C
 **********************************************************************/
static sig_atomic_t serverDone = false;
void sigIntHandler(const int)
{
    std::cout << "Caught Ctrl+C, shutting down the server..." << std::endl;
    serverDone = true;
}

/***********************************************************************
 * Launch the server
 **********************************************************************/
static int runServer(void)
{
    std::string url;
    if (optarg != NULL) url = optarg;
    if (url.empty()) url = combineURL("tcp", "::", "");

    //default url parameters when not specified
    std::string scheme, node, service; splitURL(url, scheme, node, service);
    url = combineURL(scheme.empty()?"tcp":scheme, node, service.empty()?SOAPY_REMOTE_DEFAULT_SERVICE:service);

    std::cout << uniqueProcessId() << std::endl;
    std::cout << "Launching the server... " << url << std::endl;
    SoapySocketSession sess;
    SoapyRPCSocket s;
    if (s.bind(url) != 0)
    {
        std::cerr << "Server socket bind FAIL: " << s.lastErrorMsg() << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Server bound to " << s.getsockname() << std::endl;
    s.listen(SOAPY_REMOTE_LISTEN_BACKLOG);
    auto serverListener = new SoapyServerListener(s);

    std::cout << "Press Ctrl+C to stop the server" << std::endl;
    signal(SIGINT, sigIntHandler);
    while (not serverDone) serverListener->handleOnce();

    std::cout << "Shutdown client handler threads" << std::endl;
    delete serverListener;
    s.close();

    std::cout << "Cleanup complete, exiting" << std::endl;
    return EXIT_SUCCESS;
}

/***********************************************************************
 * Parse and dispatch options
 **********************************************************************/
int main(int argc, char *argv[])
{
    std::cout << "######################################################" << std::endl;
    std::cout << "## Soapy Server -- Use any Soapy SDR remotely" << std::endl;
    std::cout << "######################################################" << std::endl;
    std::cout << std::endl;

    /*******************************************************************
     * parse command line options
     ******************************************************************/
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"bind", optional_argument, 0, 'b'},
        {0, 0, 0,  0}
    };
    int long_index = 0;
    int option = 0;
    while ((option = getopt_long_only(argc, argv, "", long_options, &long_index)) != -1)
    {
        switch (option)
        {
        case 'h': return printHelp();
        case 'b': return runServer();
        }
    }

    //unknown or unspecified options, do help...
    return printHelp();
}
