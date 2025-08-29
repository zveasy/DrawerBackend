import express from 'express';
import pinoHttp from 'pino-http';
import { json, raw, urlencoded } from 'body-parser';
import { config } from './utils/config';
import { logger } from './utils/logger';
import { webhooksRouter } from './routes/webhooks';

const app = express();

// For Square webhook signature verification, we need the raw body
app.use('/square/webhooks', raw({ type: '*/*' }));

// Normal parsing for everything else
app.use(json());
app.use(urlencoded({ extended: true }));

app.use(
  pinoHttp({
    logger,
    genReqId: (req) => (req.headers['x-request-id'] as string) || undefined,
    customSuccessMessage: function () {
      return 'request completed';
    },
  })
);

app.get('/healthz', (_req, res) => {
  res.status(200).json({ status: 'ok', env: config.NODE_ENV });
});

app.use('/square/webhooks', webhooksRouter);

const port = Number(config.PORT || 8080);
app.listen(port, () => {
  logger.info({ port }, 'API listening');
});
