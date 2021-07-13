//
// Created by Dandelonium on 7/4/2021.
//

#ifndef LAB_PARALLEL_GREP_CONFIG_H
#define LAB_PARALLEL_GREP_CONFIG_H

struct config_t {
    std::string cfg_path;
    std::string dir_path;
    std::string result;
    int indexing_threads;
    int raw_files_queue_size;
};

#endif //LAB_PARALLEL_GREP_CONFIG_H
