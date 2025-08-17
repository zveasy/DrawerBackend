#include "docs_endpoint.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace server {

static std::string docs_root() {
  const char* env = std::getenv("REGISTER_MVP_DOCS_PATH");
  if (env && *env) return env;
  return "/opt/register_mvp/share/docs";
}

static std::string mime_type(const std::string& path) {
  if (path.size() >= 5 && path.substr(path.size()-5) == ".html") return "text/html";
  if (path.size() >= 3 && path.substr(path.size()-3) == ".md") return "text/markdown";
  if (path.size() >= 4 && path.substr(path.size()-4) == ".png") return "image/png";
  if (path.size() >= 4 && path.substr(path.size()-4) == ".svg") return "image/svg+xml";
  return "text/plain";
}

static bool safe_join(const std::string& rel, fs::path& out) {
  fs::path root = docs_root();
  fs::path p = root / rel;
  auto norm = fs::weakly_canonical(p);
  auto norm_root = fs::weakly_canonical(root);
  std::string ns = norm.string();
  std::string rs = norm_root.string();
  if (ns.size() < rs.size() || ns.compare(0, rs.size(), rs) != 0) return false;
  out = norm;
  return true;
}

void register_docs_routes(httplib::Server& svr) {
  svr.Get("/help", [](const httplib::Request&, httplib::Response& res){
    fs::path root = docs_root();
    std::ostringstream oss;
    oss << "<html><body><h1>Register MVP Docs</h1><ul>";
    if (fs::exists(root)) {
      for (auto& entry : fs::directory_iterator(root)) {
        auto name = entry.path().filename().string();
        oss << "<li><a href='/help/" << name << "'>" << name << "</a></li>";
      }
    }
    oss << "</ul></body></html>";
    res.set_content(oss.str(), "text/html");
  });

  svr.Get(R"(/help/(.*))", [](const httplib::Request& req, httplib::Response& res){
    fs::path target;
    std::string rel = req.matches[1];
    if (!safe_join(rel, target)) { res.status = 404; return; }

    // If the requested path doesn't exist and ends with .html, try a .md fallback
    if (!fs::exists(target) && target.extension() == ".html") {
      fs::path md_try = target;
      md_try.replace_extension(".md");
      if (fs::exists(md_try)) {
        target = md_try;
      }
    }

    std::ifstream ifs(target, std::ios::binary);
    if (!ifs) { res.status = 404; return; }
    std::ostringstream oss; oss << ifs.rdbuf();
    res.set_content(oss.str(), mime_type(target.string()).c_str());
  });
}

} // namespace server

