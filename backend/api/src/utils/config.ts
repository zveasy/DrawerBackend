import dotenv from 'dotenv';
dotenv.config();

function requireInProduction(name: string, value: string | undefined): string {
  const resolved = value || '';
  if (process.env.NODE_ENV === 'production' && !resolved) {
    throw new Error(`${name} is required in production`);
  }
  return resolved;
}

export const config = {
  NODE_ENV: process.env.NODE_ENV || 'development',
  PORT: process.env.PORT || '8080',
  SQUARE_ENV: process.env.SQUARE_ENV || 'sandbox',
  SQUARE_APP_ID: process.env.SQUARE_APP_ID || '',
  SQUARE_ACCESS_TOKEN: process.env.SQUARE_ACCESS_TOKEN || '',
  SQUARE_WEBHOOK_SIGNATURE_KEY: requireInProduction(
    'SQUARE_WEBHOOK_SIGNATURE_KEY',
    process.env.SQUARE_WEBHOOK_SIGNATURE_KEY
  ),
  WEBHOOK_PATH: process.env.WEBHOOK_PATH || '/square/webhooks',
  WEBHOOK_BODY_LIMIT: process.env.WEBHOOK_BODY_LIMIT || '256kb',
  APP_BASE_URL: process.env.APP_BASE_URL || 'http://localhost:8080',
  DATABASE_URL: process.env.DATABASE_URL || '',
  LOG_LEVEL: process.env.LOG_LEVEL || 'info',
};
