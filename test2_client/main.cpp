#include <iostream>
#include <iomanip>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

using namespace boost;

constexpr unsigned short PORT = 8877;

constexpr int BUFF_SIZE = 1024 * 100;

class client
{

public:

    client(asio::ip::tcp::socket &socket, const std::string &file_name)
        :
        socket_(socket),
        path_(filesystem::path(file_name)),
        file_size_(filesystem::file_size(path_)),
        buff_(BUFF_SIZE, 0),
        total_bytes_(0)
    {};

    void
    process()
    {
        std::ifstream file(path_.c_str(), std::ifstream::in | std::ifstream::binary);

        if (file.good()) {

            //send file name and file size
            send_file_info();

            //whait when server ready to get file chunks
            get_ready_for_writing();

            //send buffer of file chunks
            while (file.read(buff_.data(), buff_.size())) {
                write_chunk(file.gcount());
            }
            //send remain part of file
            write_chunk(file.gcount());

            std::cout << "Sended all: " << std::boolalpha << (file_size_ == total_bytes_) << std::endl;
        }
        else {
            std::cout << "File " << path_.c_str() << " is wrong.\n";
        }
    }

private:

    void
    send_file_info()
    {
        //send to server "file_name|file_size" string
        std::cout << "File size: " << file_size_ << std::endl;

        std::stringstream ss;
        ss << path_.filename().string() << "|" << file_size_ << '\n';

        asio::write(socket_, asio::buffer(ss.str()), error_code_);

        if (error_code_) {
            std::cout << "Error while sending file info: " << error_code_ << std::endl;
        }
    }

    void
    get_ready_for_writing()
    {
        //wait server ready for file
        asio::streambuf sb;
        asio::read(socket_, sb, asio::transfer_exactly(2), error_code_);

        std::stringstream ss;
        ss.str("");
        ss << &sb;

        std::cout << "Ready: " << ss.str() << std::endl;
    }

    void
    write_chunk(std::streamsize size)
    {

        //write chunk of file
        total_bytes_ += asio::write(socket_, asio::buffer(buff_, size), error_code_);

        if (!error_code_) {
            std::cout << "Progress: "
                      << std::fixed << std::setprecision(0) << (total_bytes_ * 100.0 / file_size_)
                      << "%\r" << std::flush;
        }
        else {
            std::cout << "Error while sending file: " << error_code_ << '\n';
        }
    }

private:

    boost::asio::ip::tcp::socket &socket_;

    filesystem::path path_;

    boost::uintmax_t file_size_;

    size_t total_bytes_;

    std::vector<char> buff_;

    system::error_code error_code_;

};

int
main(int argc, const char *argv[])
{
    std::string filename;
    std::string server_ip = "127.0.0.1";
    unsigned short port = PORT;

    try {
        namespace opt = boost::program_options;

        opt::options_description desc{"Options"};
        desc.add_options()
            ("help,h", "Help")
            ("server,s", opt::value<std::string>()->default_value(server_ip), "Server IP address")
            ("port,p", opt::value<unsigned short>()->default_value(port), "Server port")
            ("file,f", opt::value<std::string>()->required(), "File to send");

        opt::variables_map vm;
        opt::store(opt::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << '\n';
            return 0;
        }

        opt::notify(vm);

        if (vm.count("server")) {
            server_ip = vm["server"].as<std::string>();
            std::cout << "Server ip: " << server_ip << '\n';
        }

        if (vm.count("port")) {
            port = vm["port"].as<unsigned short>();
            std::cout << "Server port: " << port << '\n';
        }

        if (vm.count("file")) {
            filename = vm["file"].as<std::string>();
            std::cout << "File: " << filename << '\n';
        }
    }
    catch (const program_options::error &ex) {
        std::cerr << ex.what() << '\n';
        return 0;
    }

    asio::io_service service;

    std::string address(server_ip);

    try {

        asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(address), port);

        asio::ip::tcp::socket socket(service);

        socket.open(asio::ip::tcp::v4());

        socket.connect(endpoint);

        client c(socket, filename);

        c.process();

        socket.close();
    }
    catch (std::exception &e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    std::cout << "Client finish!" << std::endl;

    return 0;
}