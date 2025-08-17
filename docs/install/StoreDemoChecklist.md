# Store Demo Checklist

1. Build the application in mock GPIO mode.
2. Start the API server:
   ```bash
   ./register_mvp --api 8080
   ```
3. In another shell, run a sample transaction:
   ```bash
   curl -s -X POST -d '{"price":500,"deposit":1000}' http://127.0.0.1:8080/txn
   ```
4. Confirm shutter motion, coins dispensed and audit log entry.

