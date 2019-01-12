/**
 * @file   yet_group.hpp
 * @author pengrl
 *
 */

#pragma once

#include <string>
#include <vector>
#include "yet_fwd.hpp"

namespace yet {

class Group {
  public:
    Group(const std::string &live_name);

    void set_http_flv_in(HttpFlvInPtr hfi);
    //HttpFlvInPtr get_in();

    void add_out(HttpFlvSessionPtr hfs);

  private:
    const std::string live_name_;
    HttpFlvInPtr hfi_;
    std::vector<HttpFlvSessionPtr> hfss_;
};

}
