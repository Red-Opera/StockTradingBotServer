package com.springboot.service;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.springboot.data.HoldingSnapshotRepository;
import com.springboot.data.PortfolioSnapshotRepository;
import com.springboot.model.Holding;
import com.springboot.model.HoldingSnapshot;
import com.springboot.model.PortfolioSnapshot;
import com.springboot.model.TradeRecord;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;

import jakarta.annotation.PreDestroy;
import jakarta.annotation.PostConstruct;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.time.LocalTime;
import java.time.DayOfWeek;
import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

@Service
public class HoldingStreamService {
    private static final Logger log = LoggerFactory.getLogger(HoldingStreamService.class);
    private volatile boolean running = true;
    private static final LocalTime DAY_START = LocalTime.MIDNIGHT;

    private final List<SseEmitter> emitters = new ArrayList<>(); // 현재 연결된 모든 SseEmitter를 저장하는 리스트
    private final Map<String, Holding> latest = new ConcurrentHashMap<>(); // 종목 코드별 최신 Holding 정보를 저장하는 맵
    private final List<TradeRecord> latestTrades = new CopyOnWriteArrayList<>(); // 최신 거래 내역을 저장하는 리스트 (메모리 전용)
    private final ObjectMapper mapper = new ObjectMapper(); // JSON 문자열을 객체로 변환하기 위한 ObjectMapper
    private final ExecutorService executor = Executors.newSingleThreadExecutor(); // 소켓 연결과 데이터 수신을 처리할 단일 스레드

    private final PortfolioSnapshotRepository portfolioSnapshotRepository;
    private final HoldingSnapshotRepository holdingSnapshotRepository;
    private final SnapshotService snapshotService;

    @Autowired
    public HoldingStreamService(PortfolioSnapshotRepository portfolioSnapshotRepository,
            HoldingSnapshotRepository holdingSnapshotRepository,
            SnapshotService snapshotService) {
        this.portfolioSnapshotRepository = portfolioSnapshotRepository;
        this.holdingSnapshotRepository = holdingSnapshotRepository;
        this.snapshotService = snapshotService;
    }

    @PostConstruct
    public void Start() {
        executor.submit(this::ConnectLoop);
    }

    @PreDestroy
    public void Stop() {
        running = false;

        executor.shutdownNow();
    }

    // 클라이언트가 SSE 스트림을 구독할 때마다 새로운 SseEmitter를 생성하여 반환하는 메서드
    public SseEmitter CreateEmitter() {
        SseEmitter emitter = new SseEmitter(Long.MAX_VALUE);

        synchronized (emitters) {
            emitters.add(emitter);
        }

        emitter.onCompletion(() -> RemoveEmitter(emitter));
        emitter.onTimeout(() -> RemoveEmitter(emitter));

        return emitter;
    }

    // SseEmitter를 리스트에서 제거하는 메서드
    private void RemoveEmitter(SseEmitter emitter) {
        synchronized (emitters) {
            emitters.remove(emitter);
        }
    }

    // 최신 보유 정보를 반환하는 메서드
    public Collection<Holding> GetLatestHoldings() {
        return latest.values();
    }

    // 최신 거래 내역을 반환하는 메서드 (메모리 전용)
    public List<TradeRecord> GetLatestTrades() {
        return new ArrayList<>(latestTrades);
    }

    // 종목코드 목록 기준 직전 거래일 종가(스냅샷 가격) 조회
    public Map<String, Long> GetPrevClosePriceMap(List<String> codes) {
        Map<String, Long> result = new LinkedHashMap<>();

        if (codes == null || codes.isEmpty())
            return result;

        LocalDateTime referenceEndTime = ResolvePrevCloseReferenceTime(LocalDateTime.now());

        List<HoldingSnapshotRepository.CodePriceProjection> rows =
                holdingSnapshotRepository.findLatestPricesByCodesBefore(codes, referenceEndTime);

        for (HoldingSnapshotRepository.CodePriceProjection row : rows)
        {
            if (row.getCode() == null)
                continue;

            result.put(row.getCode(), row.getPrice() == null ? 0L : row.getPrice());
        }

        return result;
    }

    private LocalDateTime ResolvePrevCloseReferenceTime(LocalDateTime now) {
        LocalDate date = now.toLocalDate();
        DayOfWeek dayOfWeek = date.getDayOfWeek();

        // 요구 사항 반영:
        // - 월/화/수/목/금 -> 해당 일 00:00 데이터
        // - 토/일 -> 금요일 00:00 데이터
        switch (dayOfWeek)
        {
            case SATURDAY:
                date = date.minusDays(1);
                break;

            case SUNDAY:
                date = date.minusDays(2);
                break;

            default:
                break;
        }

        return date.atTime(DAY_START);
    }

    // 데이터베이스에서 최신 보유 정보를 조회하는 메서드
    public List<HoldingSnapshot> GetLatestHoldingsFromDB() {
        return holdingSnapshotRepository.findLatestSnapshots();
    }

    // 특정 기간의 포트폴리오 스냅샷 조회
    public List<PortfolioSnapshot> GetPortfolioHistory(LocalDateTime startTime, LocalDateTime endTime) {
        if (endTime == null)
            return portfolioSnapshotRepository.findBySnapshotTimeAfterOrderBySnapshotTimeAsc(startTime);

        return portfolioSnapshotRepository.findBySnapshotTimeBetweenOrderBySnapshotTimeAsc(startTime, endTime);
    }

    // 최근 N개의 포트폴리오 스냅샷 조회
    public List<PortfolioSnapshot> GetRecentPortfolioSnapshots(int limit) {
        return portfolioSnapshotRepository.findTopNByOrderBySnapshotTimeDesc(limit);
    }

    // 시간 간격별로 샘플링된 포트폴리오 스냅샷 조회
    public List<PortfolioSnapshot> GetSampledPortfolioHistory(LocalDateTime startTime, LocalDateTime endTime,
            int intervalSeconds) {
        if (intervalSeconds <= 0)
            return GetPortfolioHistory(startTime, endTime);

        // DB 레벨에서 시간 버킷 GROUP BY로 샘플링 — Java 메모리 풀로드 방지
        return portfolioSnapshotRepository.findSampledByTimeRange(startTime, endTime, intervalSeconds);
    }

    // 새로운 Holding 정보가 수신될 때마다 모든 연결된 클라이언트에게 해당 정보를 전송하는 메서드
    private void Broadcast(Holding holding) {
        synchronized (emitters) {
            List<SseEmitter> toRemove = new ArrayList<>();

            for (SseEmitter emitter : emitters) {
                try {
                    emitter.send(SseEmitter.event().name("holding").data(holding));
                }

                catch (Exception ex) {
                    toRemove.add(emitter);
                }
            }

            emitters.removeAll(toRemove);
        }
    }

    // 거래 내역을 모든 연결된 클라이언트에게 전송하는 메서드
    private void BroadcastTrade(TradeRecord trade) {
        synchronized (emitters) {
            List<SseEmitter> toRemove = new ArrayList<>();

            for (SseEmitter emitter : emitters) {
                try {
                    emitter.send(SseEmitter.event().name("trade").data(trade));
                }

                catch (Exception ex) {
                    toRemove.add(emitter);
                }
            }

            emitters.removeAll(toRemove);
        }
    }

    private void ConnectLoop() {
        while (running) {
            try (
                    Socket socket = new Socket("localhost", 9000);
                    BufferedReader reader = new BufferedReader(
                            new InputStreamReader(socket.getInputStream(), StandardCharsets.UTF_8))) {
                log.info("Connected to localhost:9000 for holdings stream");
                String line;

                while (running && (line = reader.readLine()) != null) {
                    line = line.trim();

                    if (line.isEmpty())
                        continue;

                    try {
                        // type 필드를 확인하여 holding과 trade를 구분
                        JsonNode node = mapper.readTree(line);
                        String type = node.has("type") ? node.get("type").asText() : "holding";

                        if ("trade".equals(type)) {
                            // 거래 내역 처리
                            TradeRecord trade = mapper.treeToValue(node, TradeRecord.class);

                            log.info("Received trade: {} {} {} {}",
                                    trade.getTradeDate(), trade.getStockCode(),
                                    trade.getIoTypeName(), trade.getTradeQty());

                            // 메모리 캐시 업데이트 (동일 tradeDate+tradeNo+stockCode 중복 방지)
                            boolean exists = latestTrades.stream()
                                    .anyMatch(t -> t.getTradeDate().equals(trade.getTradeDate()) &&
                                            t.getTradeNo().equals(trade.getTradeNo()) &&
                                            t.getStockCode().equals(trade.getStockCode()));

                            if (!exists) {
                                latestTrades.add(trade);
                                BroadcastTrade(trade);
                            }
                        } else {
                            // 기존 Holding 처리
                            Holding h = mapper.treeToValue(node, Holding.class);
                            latest.put(h.getCode(), h);

                            try {
                                log.info("Received holding: {}", mapper.writeValueAsString(h));
                            }

                            catch (Exception ignore) {

                            }

                            Broadcast(h);

                            // 주기적으로 스냅샷 저장 (SnapshotService에서 간격 체크)
                            snapshotService.SaveSnapshotIfNeeded(latest);
                        }
                    }

                    catch (Exception e) {
                        log.warn("Failed to parse json: {}", line, e);
                    }
                }
            }

            catch (Exception e) {
                log.warn("Socket read error or connection failed, will retry in 2s", e);

                try {
                    Thread.sleep(2000);
                }

                catch (InterruptedException ie) {
                    Thread.currentThread().interrupt();
                    break;
                }
            }
        }
    }
}
