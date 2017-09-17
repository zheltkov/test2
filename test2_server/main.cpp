#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

using namespace boost;

constexpr unsigned short PORT = 8877;

constexpr int BUFF_SIZE = 1024 * 100;

class client_connection
{

public:

    client_connection(boost::asio::ip::tcp::socket &&socket)
        :
        socket_(std::move(socket)),
        file_name_(""),
        file_size_(0)
    {}

    ~client_connection()
    {
        socket_.close();
    }

    void
    process()
    {
        //get file name and file size form client 1-message "<file_name>|<file_size>"
        get_file_name_and_size();

        //notify client that server ready to get file content
        send_ready_to_client();

        //getting file content by buffer chunks
        get_file_content();
    }

private:

    void
    get_file_name_and_size()
    {
        asio::read_until(socket_, s_buff_, '\n');

        std::stringstream ss;
        ss << &s_buff_;
        auto info = trim_copy(ss.str());

        std::vector<std::string> v;
        split(v, info, boost::is_any_of("|"));

        if (v.size() == 2) {
            file_name_ = v[0];
            file_size_ = lexical_cast<size_t>(v[1]);
        }
        else {
            std::cout << "Receiveing error. [" << info << "]\n";
            return;
        }
        std::cout << "File name: " << file_name_ << "\nFile size:   " << file_size_ << std::endl;
    }

    void
    send_ready_to_client()
    {
        std::ostream out(&s_buff_);
        out << "ok";
        out.flush();

        asio::write(socket_, s_buff_);
    }

    void
    get_file_content()
    {
        size_t total_bytes = 0;

        system::error_code error_code;

        std::ofstream file(file_name_, std::fstream::out | std::fstream::binary);

        do {
            total_bytes += asio::read(socket_, s_buff_, asio::transfer_at_least(BUFF_SIZE), error_code);
            file << &s_buff_;
            std::cout << "Total bytes: " << total_bytes << "\r" << std::flush;
        }
        while (error_code != boost::asio::error::eof);

        std::cout << "Total bytes: " << total_bytes << std::endl;
        std::cout << "Read finish. All bytes transfered: " << std::boolalpha << (total_bytes == file_size_) << "\n\n";
    }

private:

    boost::asio::ip::tcp::socket socket_;

    asio::streambuf s_buff_;

    std::string file_name_;

    size_t file_size_;

};

class server
{

public:

    server(unsigned short port)
        : port_(port)
    {}

    void
    accept_connectins()
    {
        asio::io_service service;

        try {
            asio::ip::tcp::acceptor acceptor(service, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port_));

            for (;;) {
                asio::ip::tcp::socket socket(service);

                std::cout << "Accept connections...\n";
                acceptor.accept(socket);

                client_connection c(std::move(socket));
                c.process();
            }
        }
        catch (std::exception &e) {
            std::cout << "Some exception: " << e.what() << std::endl;
        }
    }

private:
    unsigned short port_;
};

int
main(int argc, const char *argv[])
{
    unsigned short port = PORT;

    try {
        namespace opt = boost::program_options;

        opt::options_description desc{"Options"};
        desc.add_options()
            ("help,h", "Help")
            ("port,p", opt::value<unsigned short>()->default_value(port), "Server port");

        opt::variables_map vm;
        opt::store(opt::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << '\n';
            return 0;
        }

        opt::notify(vm);

        if (vm.count("port")) {
            port = vm["port"].as<unsigned short>();
            std::cout << "Server port: " << port << '\n';
        }
    }
    catch (const program_options::error &ex) {
        std::cerr << ex.what() << '\n';
        return 0;
    }

    server s(port);
    s.accept_connectins();

    std::cout << "Server stopped." << std::endl;
    return 0;
}