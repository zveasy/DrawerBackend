# Security Hardening

This document outlines the baseline hardening applied to the device.

* Read only root via overlayfs
* Least privilege systemd unit
* SSH key-only authentication
* nftables firewall deny-by-default
* auditd rules for key files
* Metrics endpoints require authentication
