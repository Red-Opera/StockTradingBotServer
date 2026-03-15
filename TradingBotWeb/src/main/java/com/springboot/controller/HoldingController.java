package com.springboot.controller;

import com.springboot.model.Holding;
import com.springboot.model.HoldingSnapshot;
import com.springboot.model.PortfolioSnapshot;
import com.springboot.model.TradeRecord;
import com.springboot.service.HoldingStreamService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.format.annotation.DateTimeFormat;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;

import java.time.DayOfWeek;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.time.LocalTime;
import java.util.Collection;
import java.util.List;

@CrossOrigin(origins = "*")
@RestController
@RequestMapping("/stream")
public class HoldingController {
    private final HoldingStreamService service;

    @Autowired
    public HoldingController(HoldingStreamService service) {
        this.service = service;
    }

    @GetMapping(value = "/holdings", produces = MediaType.TEXT_EVENT_STREAM_VALUE)
    public SseEmitter StreamHoldings() {
        // 각 클라이언트마다 새로운 SseEmitter를 생성하여 반환
        return service.CreateEmitter();
    }

    @GetMapping("/holdings/latest")
    public Collection<Holding> GetLastest() {
        // 최신 보유 정보를 반환하는 엔드포인트 (메모리에서)
        return service.GetLatestHoldings();
    }

    // 데이터베이스에서 최신 보유 정보 조회
    @GetMapping("/holdings/latest/db")
    public List<HoldingSnapshot> GetLatestFromDB() {
        return service.GetLatestHoldingsFromDB();
    }

    // 포트폴리오 이력 조회 (특정 기간)
    @GetMapping("/portfolio/history")
    public List<PortfolioSnapshot> GetPortfolioHistory(
            @RequestParam @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) LocalDateTime startTime,
            @RequestParam(required = false) @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) LocalDateTime endTime) {
        return service.GetPortfolioHistory(startTime, endTime);
    }

    // 포트폴리오 최근 N개 스냅샷 조회
    @GetMapping("/portfolio/recent")
    public List<PortfolioSnapshot> GetRecentPortfolioSnapshots(
            @RequestParam(defaultValue = "100") int limit) {
        // 최대 1000개로 제한
        if (limit > 1000)
            limit = 1000;

        return service.GetRecentPortfolioSnapshots(limit);
    }

    // 포트폴리오 최근 24시간 데이터 조회 (편의 메서드)
    @GetMapping("/portfolio/recent24h")
    public List<PortfolioSnapshot> GetRecent24Hours() {
        LocalDateTime startTime = LocalDateTime.now().minusHours(24);

        return service.GetPortfolioHistory(startTime, null);
    }

    // 포트폴리오 최근 7일 데이터 조회 (편의 메서드)
    @GetMapping("/portfolio/recent7d")
    public List<PortfolioSnapshot> GetRecent7Days() {
        LocalDateTime startTime = LocalDateTime.now().minusDays(7);

        return service.GetPortfolioHistory(startTime, null);
    }

    // 장 운영 시간 상수
    private static final LocalTime MARKET_OPEN = LocalTime.of(9, 0);
    private static final LocalTime MARKET_CLOSE = LocalTime.of(15, 30);

    /**
     * 현재 시간이 장 운영 시간 외일 때, 가장 최근 장 마감 시각(평일 15:30)을 반환.
     * 장 운영 시간 내이면 현재 시각을 반환.
     */
    private LocalDateTime getEffectiveEndTime() {
        LocalDateTime now = LocalDateTime.now();
        LocalDate today = now.toLocalDate();
        DayOfWeek dow = today.getDayOfWeek();
        LocalTime currentTime = now.toLocalTime();

        // 평일 장 운영 시간 내
        if (dow != DayOfWeek.SATURDAY && dow != DayOfWeek.SUNDAY
                && !currentTime.isBefore(MARKET_OPEN) && !currentTime.isAfter(MARKET_CLOSE)) {
            return now;
        }

        // 평일이고 장 마감 후 → 오늘 15:30
        if (dow != DayOfWeek.SATURDAY && dow != DayOfWeek.SUNDAY
                && currentTime.isAfter(MARKET_CLOSE)) {
            return today.atTime(MARKET_CLOSE);
        }

        // 그 외(주말 또는 평일 장 시작 전) → 가장 최근 평일의 15:30
        LocalDate date = today;
        if (currentTime.isBefore(MARKET_OPEN) || dow == DayOfWeek.SATURDAY || dow == DayOfWeek.SUNDAY) {
            date = date.minusDays(1);
        }
        while (date.getDayOfWeek() == DayOfWeek.SATURDAY || date.getDayOfWeek() == DayOfWeek.SUNDAY) {
            date = date.minusDays(1);
        }
        return date.atTime(MARKET_CLOSE);
    }

    // 포트폴리오 차트용 데이터 조회 (지정된 범위에 따라 다운샘플링)
    @GetMapping("/portfolio/chart")
    public List<PortfolioSnapshot> GetPortfolioChartData(@RequestParam(defaultValue = "day") String range) {
        LocalDateTime now = LocalDateTime.now();
        LocalDateTime endTime;
        LocalDateTime startTime;
        int intervalSeconds;

        switch (range.toLowerCase()) {
            case "second":
                // 최근 5분, 5초 간격 (최대 60개) — 장 마감 시 마감 시점 기준
                endTime = getEffectiveEndTime();
                startTime = endTime.minusMinutes(5);
                intervalSeconds = 5;
                break;
            case "minute":
                // 최근 1시간, 1분 간격 (최대 60개) — 장 마감 시 마감 시점 기준
                endTime = getEffectiveEndTime();
                startTime = endTime.minusHours(1);
                intervalSeconds = 60;
                break;
            case "hour":
                // 최근 24시간, 10분 간격 (최대 144개) — 장 마감 시 마감 시점 기준
                endTime = getEffectiveEndTime();
                startTime = endTime.minusHours(24);
                intervalSeconds = 600;
                break;
            case "day":
                // 최근 7일, 1시간 간격 (최대 168개)
                endTime = now;
                startTime = now.minusDays(7);
                intervalSeconds = 3600;
                break;
            case "week":
                // 최근 1개월, 6시간 간격 (최대 120개)
                endTime = now;
                startTime = now.minusMonths(1);
                intervalSeconds = 21600;
                break;
            case "month":
                // 최근 6개월, 1일 간격 (최대 180개)
                endTime = now;
                startTime = now.minusMonths(6);
                intervalSeconds = 86400;
                break;
            default:
                // 기본값: day
                endTime = now;
                startTime = now.minusDays(7);
                intervalSeconds = 3600;
                break;
        }

        return service.GetSampledPortfolioHistory(startTime, endTime, intervalSeconds);
    }

    // 최신 거래 내역 조회 (메모리 캐시)
    @GetMapping("/trades/latest")
    public List<TradeRecord> GetLatestTrades() {
        return service.GetLatestTrades();
    }
}
