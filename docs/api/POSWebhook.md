# POS Webhook

The register can send updates to an external POS via HTTP webhook.

- **Endpoint:** configured URL in `config.ini`.
- **Payload:**
  ```json
  {"id":"123","status":"OK","change":265}
  ```
- Webhook must respond `200` within 2s. Non-200 responses are retried with
  exponential backoff (max 5 attempts).
- Clients must handle duplicate delivery; `id` is unique per transaction.

Error codes mirror the local API and include `BUSY`, `NOCHANGE` and
`CONFIG_ERROR`.

