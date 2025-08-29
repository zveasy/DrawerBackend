import dotenv from 'dotenv';
dotenv.config();

export const config = {
  NODE_ENV: process.env.NODE_ENV || 'development',
  PORT: process.env.PORT || '8080',
  SQUARE_ENV: process.env.SQUARE_ENV || 'sandbox',
  SQUARE_APP_ID: process.env.SQUARE_APP_ID || '',
  SQUARE_ACCESS_TOKEN: process.env.SQUARE_ACCESS_TOKEN || '',
  SQUARE_WEBHOOK_SIGNATURE_KEY: process.env.SQUARE_WEBHOOK_SIGNATURE_KEY || '',
  WEBHOOK_PATH: process.env.WEBHOOK_PATH || '/square/webhooks',
  APP_BASE_URL: process.env.APP_BASE_URL || 'http://localhost:8080',
  DATABASE_URL: process.env.DATABASE_URL || '',
  LOG_LEVEL: process.env.LOG_LEVEL || 'info',
};
