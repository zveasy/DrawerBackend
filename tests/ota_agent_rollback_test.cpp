#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <map>
#include <iomanip>
#include <sstream>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/sha.h>
#include "ota/agent.hpp"
#include "ota/healthd.hpp"
#include "util/persist.hpp"
#include <httplib.h>

using namespace std::filesystem;

static std::string pub_pem(EVP_PKEY* pkey){ BIO* b=BIO_new(BIO_s_mem()); PEM_write_bio_PUBKEY(b,pkey); BUF_MEM* mem; BIO_get_mem_ptr(b,&mem); std::string s(mem->data, mem->length); BIO_free(b); return s; }
static std::string sign_msg(EVP_PKEY* pkey,const std::string& msg){ EVP_MD_CTX* c=EVP_MD_CTX_new(); EVP_DigestSignInit(c,nullptr,nullptr,nullptr,pkey); size_t len=0; EVP_DigestSign(c,nullptr,&len,(const unsigned char*)msg.data(),msg.size()); std::string sig(len,'\0'); EVP_DigestSign(c,(unsigned char*)sig.data(),&len,(const unsigned char*)msg.data(),msg.size()); EVP_MD_CTX_free(c); BIO* b64=BIO_new(BIO_f_base64()); BIO_set_flags(b64,BIO_FLAGS_BASE64_NO_NL); BIO* mem=BIO_new(BIO_s_mem()); b64=BIO_push(b64,mem); BIO_write(b64,sig.data(),len); BIO_flush(b64); BUF_MEM* buf; BIO_get_mem_ptr(b64,&buf); std::string out(buf->data,buf->length); BIO_free_all(b64); return out; }
static std::string hex_sha(const std::string& d){ unsigned char h[SHA256_DIGEST_LENGTH]; SHA256((const unsigned char*)d.data(),d.size(),h); std::ostringstream o; o<<std::hex<<std::setfill('0'); for(int i=0;i<SHA256_DIGEST_LENGTH;i++) o<<std::setw(2)<<(int)h[i]; return o.str(); }

TEST(OtaAgent, Rollback) {
  path tmp = temp_directory_path()/"ota_rollback"; remove_all(tmp); create_directories(tmp);
  path state = tmp/"state"; create_directories(state);
  std::map<std::string,std::string> init{{"current_version","1.0.0"},{"boot_pending","0"}}; persist::save_kv((state/"state.json").string(), init);

  EVP_PKEY_CTX* pctx=EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519,nullptr); EVP_PKEY* pkey=nullptr; EVP_PKEY_keygen_init(pctx); EVP_PKEY_keygen(pctx,&pkey);
  std::string pub=pub_pem(pkey); std::ofstream(state/"pub.pem")<<pub;

  path artifact = tmp/"artifact.bin"; std::string adata="bad"; std::ofstream(artifact)<<adata;
  std::string sha = hex_sha(adata);
  std::string payload = std::string("{\"channel\":\"stable\",\"version\":\"1.2.0\",\"artifact_url\":\"file://") + artifact.string() + "\",\"sha256\":\""+sha+"\"}";
  std::string sig = sign_msg(pkey, payload);
  std::string manifest = payload.substr(0,payload.size()-1) + ",\"sig_ed25519\":\""+sig+"\"}";
  path feed = tmp/"feed"; create_directories(feed); std::ofstream(feed/"release.json")<<manifest;

  cfg::Config c = cfg::defaults();
  c.ota.enable=true; c.ota.feed_url="file://"+(feed/"release.json").string(); c.ota.channel="stable"; c.ota.state_dir=state.string(); c.ota.key_pub=(state/"pub.pem").string(); c.ota.require_signed=0;

  auto backend = ota::make_local_backend((state/"backend").string());
  ota::Agent agent(c,*backend); auto res = agent.run_once(); ASSERT_TRUE(res.ok);
  httplib::Server svr; svr.Get("/healthz",[](const httplib::Request&,httplib::Response& r){ r.status=500; });
  int port = svr.bind_to_any_port("127.0.0.1"); std::thread th([&]{ svr.listen_after_bind(); });
  ota::Healthd hd(c,*backend); auto res2 = hd.run_once("http://127.0.0.1:"+std::to_string(port)); svr.stop(); th.join(); EXPECT_FALSE(res2.ok);

  std::map<std::string,std::string> kv; persist::load_kv((state/"state.json").string(),kv);
  EXPECT_EQ(kv["current_version"],"1.0.0");
  EXPECT_EQ(kv["boot_pending"],"0");
  EXPECT_NE(kv["last_bad"].find("1.2.0"), std::string::npos);

  EVP_PKEY_free(pkey); EVP_PKEY_CTX_free(pctx);
}

