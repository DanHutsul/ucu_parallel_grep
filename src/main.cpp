#include <iostream>
#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#include <archive.h>
#include <archive_entry.h>
#include <unordered_map>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>
#include <thread>
#include "../inc/config.h"
#include "../inc/precise_time.h"
#include <boost/tokenizer.hpp>
#include <queue>

#define IO_READ_ERR 7
#define INVALID_CFG_ERR 8

namespace bl = boost::locale;
namespace blb = boost::locale::boundary;
namespace bfs = boost::filesystem;




// For concurrent_hash_map
struct my_hash {
    static size_t hash(const std::string& key) {
        size_t h = 0;
        for(const char* s = key.c_str(); *s; ++s)
            h = (h*17)^*s;
        return h;
    }
    //! True if strings are equal
    static bool equal(const std::string& key1, const std::string& key2) {
        return key1 == key2;
    }
};

class value;

void read_config(config_t &config) {

    std::ifstream cfg(config.cfg_path);

    if (!cfg.is_open()) {
        exit(IO_READ_ERR);
    }

    std::string line;
    std::unordered_map<std::string, std::string> config_data;
    while (std::getline(cfg, line)) {
        if (line.find('=') != std::string::npos) {
            std::istringstream iss{line};
            std::string key, value;

            if (std::getline(std::getline(iss, key, '=') >> std::ws, value)) {
                key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
                config_data[key] = value;
            }
        }
    }
    cfg.close();

    if (config_data["path"].empty() || config_data["res_file"].empty() || config_data["indexing_threads"].empty() ||
        config_data["raw_files_queue_size"].empty())
    {
        std::cout << "Write the appropriate config!" << std::endl;
        exit(INVALID_CFG_ERR);
    }
    config.dir_path = config_data["path"];
    config.result = config_data["res_file"];
    config.indexing_threads = std::stoi(config_data["indexing_threads"]);
    config.raw_files_queue_size = std::stoi(config_data["raw_files_queue_size"]);
}


std::string read_binary_file(const std::string& file_name) {
    std::ifstream raw_file(file_name, std::ios::binary);
    std::ostringstream buffer_ss;
    buffer_ss << raw_file.rdbuf();
    std::string buffer{buffer_ss.str()};
    raw_file.close();
    return buffer;
}

// out_filename must be a vector or a queue
void unzip(std::string &raw_data, std::vector <std::string> &result, tbb::concurrent_bounded_queue<std::string> &out_filename){
    struct archive *arch;
    struct archive_entry *entry;
    int error_code;

    arch = archive_read_new();
    archive_read_support_filter_all(arch);
    archive_read_support_format_all(arch);

    error_code = archive_read_open_memory(arch, &raw_data[0], raw_data.size());

    if (error_code != ARCHIVE_OK) {
        std::cout << "Cannot open archive" << std::endl;
        return;
    }

    while (archive_read_next_header(arch, &entry) == ARCHIVE_OK) {
        std::string filename = archive_entry_pathname(entry);
        std::string curr_extension = bl::to_lower(filename.substr(filename.find_last_of(".") + 1));
        if (curr_extension != "txt") {
            continue;
        }
        auto file_size = archive_entry_size(entry);
        if (file_size > 10'000'000){
            continue;
        }
        // Idk if this error means anything
        out_filename.push(filename);
        std::string res(file_size, char{});
        archive_read_data(arch, &res[0], file_size);
        result.push_back(res);
    }
    archive_read_free(arch);
}

// This is changed
//
void filter_and_index(std::string &file_content, std::string &file_name, std::string &to_find, tbb::concurrent_hash_map<std::string, std::map<int, std::string>, my_hash> &maps_to_merge) {
    // We have a map that maps *File name* to a *normal map* that contains pairs of Line number and its Contents
    int line = 1;
    typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
    boost::char_separator<char> sep("\n");
    tokenizer tok{file_content, sep};
    for (const auto &t : tok) {
        if (t.find(to_find) != std::string::npos) {
            // Insert into concurrent_hash_map
            //std::cout << "found at line " << line << '\n';
            //std::cout << line << ": " << t << '\n';
            tbb::concurrent_hash_map<std::string, std::map<int, std::string>, my_hash>::accessor a;
            maps_to_merge.insert(a, file_name);
            a->second.insert({line, t});
        }
        line++;
    }
}


void read_archives_into_queue(std::string& dir_path, tbb::concurrent_bounded_queue<std::string>& archives_to_unzip, tbb::concurrent_bounded_queue<std::string>& file_paths) {
    // We need to create a vector of strings
    // It will hold full path of file
    // The vector must be thread-safe
    bfs::recursive_directory_iterator end_itr;

    for (bfs::recursive_directory_iterator itr(dir_path); itr != end_itr; itr++) {
        // path is our full path
        std::string path = itr->path().string();

        if (bl::to_lower(path.substr(path.find_last_of(".") + 1)) == "zip") {
            // At this point we have our .zip path
            // So we push it
            file_paths.push(path);
            std::string buff = read_binary_file(path);
            archives_to_unzip.push(buff);
        }
    }
    //push poison pill
    archives_to_unzip.push("");
}

void process_raw_data(tbb::concurrent_bounded_queue<std::string> &archives_to_unzip,
                      tbb::concurrent_hash_map<std::string, std::map<int, std::string>, my_hash> &maps_to_merge,
                      config_t &config, std::string &to_find, tbb::concurrent_bounded_queue<std::string>& file_paths) {
    for (;;) {
        std::string temp;
        std::string temp_file_path;
        if (archives_to_unzip.try_pop(temp)){
            if (temp.empty()) {
                archives_to_unzip.push("");
                return;
            }
            file_paths.try_pop(ref(temp_file_path));
        }
        std::string file_path = temp_file_path;
        std::string raw_data = temp;
        std::vector <std::string> unzipped_files;
        // Unzip must have some way to get us file names
        // We must get them into a vector?
        // Queue is better
        tbb::concurrent_bounded_queue <std::string> file_name;
        // And parse a reference of it to function
        unzip(raw_data, unzipped_files, ref(file_name));
        // Create string to give to filter function
        for (auto & unzipped_file : unzipped_files) {
            std::string file_path_name;
            // Fill the string
            file_name.pop(ref(file_path_name));
            std::string full_path = file_path + "/" + file_path_name;
            // Then parse a reference of it to this function
            filter_and_index(unzipped_file, ref(full_path), to_find, maps_to_merge);
        }
        unzipped_files.clear();
    }
}


bool sort_by_name(const std::pair<std::string, std::map<int, std::string>> &a,
                  const std::pair<std::string, std::map<int, std::string>> &b)
{
    return (a.first < b.first);
}

int main(int argc, char *argv[]) {

    auto total_start = get_current_time_fenced();

    std::string cfg_path = "../config/config.dat";

    std::string string_to_find;

    // Uncomment when done testing
    if (argc > 3) {
        std::cerr << "Too many arguments" << std::endl;
        return -1;
    }
    else if (argc < 2) {
        std::cerr << "Too few arguments\nNo string inputted" << std::endl;
        return -1;
    }
    else {
        string_to_find = argv[1];
        if (argc > 2) {
            cfg_path =  argv[2];
        }
    }

    // Remove later
    //string_to_find = "Jim";

    config_t config;
    config.cfg_path = cfg_path;
    read_config(config);


    bl::generator gen;
    auto locale_en = gen("en_US.UTF-8");
    std::locale::global(locale_en);

    //////////////////////////////////////////////////////////////////////


    tbb::concurrent_bounded_queue<std::string> archives_to_unzip;
    archives_to_unzip.set_capacity(config.raw_files_queue_size);

    tbb::concurrent_hash_map<std::string, std::map<int, std::string>, my_hash> maps_to_merge;

    tbb::concurrent_bounded_queue<std::string> file_paths;

    std::vector <std::thread> vec_of_threads;

    auto search_reading_start = get_current_time_fenced();

    vec_of_threads.emplace_back(read_archives_into_queue, ref(config.dir_path), ref(archives_to_unzip), ref(file_paths));

    std::cout << 1 <<std::endl;//temporary prints to see where program works at the moment

    //std::cout << config.indexing_threads <<std::endl;
    for (size_t i = 0; i < config.indexing_threads; i++) {
        vec_of_threads.emplace_back(process_raw_data, ref(archives_to_unzip), ref(maps_to_merge), std::ref(config), ref(string_to_find), ref(file_paths));
    }

    std::cout << 2 <<std::endl;


    size_t counter = 0;
    auto search_reading_finish = get_current_time_fenced(), indexing_finish = get_current_time_fenced();
    //
    for (auto &thread: vec_of_threads) {
        std::cout << "Next thread to be joined: " << counter << std::endl; // temp prints to check threads
        thread.join();
        if (counter == 0) {
            search_reading_finish = get_current_time_fenced();
        }
        else if (counter == config.indexing_threads) {
            indexing_finish = get_current_time_fenced();
        }
        std::cout << "Thread joined: " << counter << std::endl;
        counter++;
    }
    std::cout << 4 << std::endl;

    vec_of_threads.clear();

    auto total_finish = get_current_time_fenced();

    // Have to change std::map to std::vector of std::pair
    std::vector <std::pair <std::string, std::map<int, std::string>> > results;

    std::cout << 5 <<std::endl;


    for (auto & pair: maps_to_merge){
        results.emplace_back(pair.first, pair.second);
    }

    // Output, can be repurposed to write file
    /*
    for (auto & pair: results) {
        std::cout << pair.first << std::endl;
        for (auto & map_pair: pair.second) {
            std::cout << map_pair.first << ":" << map_pair.second << std::endl;
        }
    }*/




    std::cout << 6 <<std::endl;

    std::sort(results.begin(), results.end(), sort_by_name);



    std::cout << 7 <<std::endl;

    bfs::create_directory("../results");
    // change result_by_amount to just result
    std::ofstream res_out("../results/" + config.result);

    /*for (const auto& pair : results) {
        // I don't think I need to sort the map
        // Elements are inserted in order
        res_out << pair.first << ":" << std::endl;
        for (const auto& [key, value]: pair.second) {
            res_out << key << ": " << value << std::endl;
        }
    }*/

    for (auto & pair: results) {
        res_out << pair.first << std::endl;
        for (auto & map_pair: pair.second) {
            res_out << map_pair.first << ":" << map_pair.second << std::endl;
        }
    }


    std::cout << 8 <<std::endl;
    res_out.close();
    /*
    std::sort(result2.begin(), result2.end(), sort_by_name);

    std::cout << 9 << std::endl;

    std::ofstream res_out("../results/" + config.result_by_name);
    for (const auto& pair : result2) {
        res_out << pair.first << " " << pair.second << std::endl;
    }

    std::cout << 10 << std::endl;

    res_out.close();
    res_out2.close();*/
    auto total_time = total_finish - total_start;
    auto reading_time = search_reading_finish - search_reading_start;
    auto reading_indexing_time = indexing_finish - search_reading_start;

    std::cout << "Reading and indexing time = " << (double) to_us(reading_indexing_time) / 1e6 << " s" << std::endl;
    std::cout << "Reading time = " << (double) to_us(reading_time) / 1e6 << " s" << std::endl;
    std::cout << "Total time = " << (double) to_us(total_time) / 1e6 << " s" << std::endl;

    std::ofstream out_total_time("../results/total_time.txt");
    out_total_time << (double) to_us(total_time) / 1e6 << std::endl;
    out_total_time.close();

    return 0;
}
