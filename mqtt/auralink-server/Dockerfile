FROM golang:1.24-alpine AS builder

WORKDIR /app
COPY . .
RUN go build -o auralink-server ./cmd/auralink-server

FROM alpine:latest

WORKDIR /root/
COPY --from=builder /app/auralink-server .
COPY config.yaml .
EXPOSE 8080
CMD ["./auralink-server", "--config", "config.yaml"]
