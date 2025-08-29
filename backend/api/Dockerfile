# syntax=docker/dockerfile:1
FROM node:20-alpine AS base

WORKDIR /app
ENV NODE_ENV=production

RUN apk add --no-cache openssl

COPY package.json package-lock.json* ./
# Install ALL dependencies for build (dev + prod)
RUN if [ -f package-lock.json ]; then \
      npm ci; \
    else \
      npm install; \
    fi

COPY prisma ./prisma
RUN npx prisma generate

COPY tsconfig.json ./
COPY src ./src
RUN npm run build

# Prune dev dependencies after build so runtime is slim
RUN npm prune --omit=dev

# Runtime image
FROM node:20-alpine
WORKDIR /app
ENV NODE_ENV=production
RUN apk add --no-cache openssl

COPY --from=base /app/node_modules ./node_modules
COPY --from=base /app/prisma ./prisma
COPY --from=base /app/dist ./dist
COPY --from=base /app/package.json ./package.json
COPY entrypoint.sh ./entrypoint.sh
RUN chmod +x /app/entrypoint.sh

ENV PORT=8080
EXPOSE 8080
CMD ["/app/entrypoint.sh"]
