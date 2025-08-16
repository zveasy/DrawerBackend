# SLOs

| Name | Target | Window | PromQL |
|------|--------|--------|--------|
| Success rate 30m | 99.5% | 30m | `increase(register_txn_total{status="OK"}[30m]) / increase(register_txn_total[30m])` |
| Jam rate per 1k | <=1/1000 | 1h | `increase(register_jam_total[1h]) / increase(register_txn_total[1h])` |

Decision policy: page on any SLO breach, ticket for LowCoins.
