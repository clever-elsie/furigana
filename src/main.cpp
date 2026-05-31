#include <args.hpp>
#include <recurse.hpp>
#include <commit.hpp>

int main(int argc, char**argv){
  const config_t conf = proc_args(argc, argv);
  const auto list = get_directories_list(argc, argv);
  for(const auto&dir:list){
    std::vector<result_t> checked = recurse(dir);
    commit(checked);
  }
}