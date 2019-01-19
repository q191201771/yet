/**
 * @file   yet_group.h
 * @author pengrl
 *
 */

#pragma once

#include <string>
#include <vector>
#include <set>
#include "yet_fwd.hpp"
#include "http_flv_pull.h"

namespace yet {

class Group : public std::enable_shared_from_this<Group> {
  public:
    Group(const std::string &live_name);

    void set_http_flv_pull(HttpFlvPullPtr pull);
    HttpFlvPullPtr get_in();

    void add_out(HttpFlvSubPtr sub);
    void del_out(HttpFlvSubPtr sub);

    BufferPtr get_metadata();
    BufferPtr get_video_seq_header();

  public:
    void on_connected();
    void on_data(BufferPtr buf, const std::vector<FlvTagInfo> &tis);

  private:
    const std::string live_name_;
    HttpFlvPullPtr pull_;
    std::set<HttpFlvSubPtr> subs_;
};

}
