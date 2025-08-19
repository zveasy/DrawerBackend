#pragma once
#include <string>
#include <filesystem>
#include <fstream>

// Write a self-signed certificate and key to the given directory.
// Paths to the created files are returned via cert and key arguments.
inline void write_test_cert(const std::filesystem::path& dir,
                            std::string& cert,
                            std::string& key) {
  static const char kCert[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIDCTCCAfGgAwIBAgIUegXfz3qoKc5wSg64BLsBWLfbLqEwDQYJKoZIhvcNAQEL\n"
"BQAwFDESMBAGA1UEAwwJbG9jYWxob3N0MB4XDTI1MDgxOTE5MDgxNloXDTI1MDgy\n"
"MDE5MDgxNlowFDESMBAGA1UEAwwJbG9jYWxob3N0MIIBIjANBgkqhkiG9w0BAQEF\n"
"AAOCAQ8AMIIBCgKCAQEA7B839XOxi2PrrI6qSCmuhRUPyrHdJH95mb71UmCMGedZ\n"
"kBNBInR+IioPxLDPh27Z9B3d3+cXqDDOPKcnoxc8nz8RKkd5HdHOSb56wQ4g8S1X\n"
"Wz3QcGAruB+aos2Dy+uxEK+o61oxBgDT9BFyjoPfCcay+AGx52nYpdfKBF1glQnz\n"
"W+J/3Lhn+NAqHLXz6i0prfmnkJtnbqwllxhTPVytawX39lisLCjByd4vhfbqPc6H\n"
"7sifT4apekqwwvswEU78uNZAuyCob1v67nA2P3TCwCBG6FKTABUoyyaIjyahVb0q\n"
"+wx8XHW2XdWFfWLjxz8EaAlCKIX4WtZtwB4RwWk7uwIDAQABo1MwUTAdBgNVHQ4E\n"
"FgQUEMvKB3MLmtEwmJ+/j5zIYMuc38kwHwYDVR0jBBgwFoAUEMvKB3MLmtEwmJ+/\n"
"j5zIYMuc38kwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAU/3x\n"
"80sZ1yWFOaqA0BtjG8BXZIMxhr7qFQs0RIatwYyf+criSA0AJla/gCjkvuYeJF0l\n"
"yQ+Idzi4Cg3bptR0vzV2nFcZh0DUnc4QT6x2DxgW2g7baob5vi7vEu4LI2EHdqNB\n"
"fOCGEBmiOxKKKCjcw+3Ro76Wqe+mDIGkpr8aPrJvQZpdXj03GVkwN+Gfx7Zo9ruc\n"
"rbKpZ06jkubMZSdkF3UPC1kdes1MqpQK9btWYf1yHvdvkGXfUB4IT2jvGP6kn1Pj\n"
"Ew0n5v8XQrUx7O1pGlDcQG69n9Mj5A7KL+nWMUM64AK6YT5BUMt/1zhhXYeyjjFU\n"
"T9QR7rfpw+CXZgA/Kg==\n"
"-----END CERTIFICATE-----\n";

  static const char kKey[] =
"-----BEGIN PRIVATE KEY-----\n"
"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDsHzf1c7GLY+us\n"
"jqpIKa6FFQ/Ksd0kf3mZvvVSYIwZ51mQE0EidH4iKg/EsM+Hbtn0Hd3f5xeoMM48\n"
"pyejFzyfPxEqR3kd0c5JvnrBDiDxLVdbPdBwYCu4H5qizYPL67EQr6jrWjEGANP0\n"
"EXKOg98JxrL4AbHnadil18oEXWCVCfNb4n/cuGf40CoctfPqLSmt+aeQm2durCWX\n"
"GFM9XK1rBff2WKwsKMHJ3i+F9uo9zofuyJ9Phql6SrDC+zARTvy41kC7IKhvW/ru\n"
"cDY/dMLAIEboUpMAFSjLJoiPJqFVvSr7DHxcdbZd1YV9YuPHPwRoCUIohfha1m3A\n"
"HhHBaTu7AgMBAAECggEAUDUSlygjqUvZ5JXZtgWiqMZMxPfCPQGoVteNEdyF+s6h\n"
"l9VSjNexeP18ub2t4T2Af/IdSk9/s7xQcj39suLTzuxncksxEzYPsvEnVajs+8AB\n"
"KpdG1MV2VGc36hGRsZVwYlCpOrk6aeWiGghLN6oH+5QpeyFoQ0mrNDqm/vCRPE6r\n"
"+SSaGoGyj+cBDW2cMq1TXlOQ8kl6dHjvzz6rEgZpH5e6RJ4rx493l8qxucT2tWYj\n"
"Nmpix6TILtBXJbeEVimyLsacC7taWBDblFsPdKZotvSNgN6PczkYfunJ/HTZXiq/\n"
"fXVx7uJy6LdKRTMRJnodfNkQgxMRkx3pElsfcPSpgQKBgQD2/CBEvw+1Ey7ZfNMt\n"
"o7ac3sACbEzYmdtk5pjJkJ1tQTO7vroJPwuaX4xpjD1I4hlTV+/Eusqve/b9n17p\n"
"dm2NAH7PzgrZrLzagxnVYf0l2B0+DkqLHq4e2LbmKqi8sD/Nf5jeoHCz2q93TZcl\n"
"SFT4Z96/BtCdZEzhE4vdqfahoQKBgQD0vZZbmbeLNKLYSilcuvUoeDW4fEumy/Hk\n"
"CVw1PPSp6XXxSHkHXX6twpAJZxmBxtOgN6jXbghEAiOEhRJlrht1X1O0syKSNgQS\n"
"lUHGClEHNtZXssWJmKyFhQcAl8rlL82odL+K01vm2rPyZgMBnJF0riVYSIMkM2AI\n"
"0ZjUWLeX2wKBgQCgl72Plb/r2EZNKgnSEjIp+/hTWwH4kMoD6KCN51dFc/DkcZZb\n"
"br/np5sQAhzTKBiZhYMkouQpiGxH6vl2ygdfeGP8UJfjg5rkZfxFL8q/ca9J61by\n"
"8Ib9DaKXNEO1NNC3mPDYSPAfMeGHrE7L8iU1w6wk/5Rj0pTegKwf/GSeIQKBgQCH\n"
"RU6YvIKdN2+Weo8YCOG+B4sxt8mcnHbLIn1Lk7BrRXWB8huq/XsqETLJb6nCeCG6\n"
"GWmCAPgilsgI1ABSIQReQF0ksCo4hBCGMOcUzdjxUtbvzAiwv6kDd35iToO/X0ed\n"
"h0HjOmU+WL1DCi05M8+VnTdY72NEm/zFgwukupBnJwKBgHLv/pb1yjKYNjH/quVc\n"
"Kel6c2TrUpMRBNSNTFcJf/LfcZ1/ElB0L/bqo3XPFdMeBlsdEauwxkg/rgMuvr0E\n"
"NX4TPVlOlKjVFjZU7H21CGh65+Ixh7b5HP3VKszcxImI0s2DOQjk5WTwQb+QRGp7\n"
"XJA61z2gxEg30fxai6DC4G5Y\n"
"-----END PRIVATE KEY-----\n";

  std::filesystem::create_directories(dir);
  cert = (dir / "cert.pem").string();
  key  = (dir / "key.pem").string();
  std::ofstream(cert) << kCert;
  std::ofstream(key)  << kKey;
}

