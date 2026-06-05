#include <args.hpp>

const mutable_config_t default_config(
  false, // yes
  false, // check_already_has_ruby
  false, // https
  "localhost",  // ip
  11434,        // port
  1,            // batch
  65536,        // num_ctx
  10min,        // timeout
  "gemma4:12b", // generate_model
  "gemma4:12b", // check_model
  "",           // wordset
  std::vector<std::filesystem::path>() // dirlist
);
