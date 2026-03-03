package com.springboot.controller;

import com.springboot.model.Holding;
import com.springboot.model.HoldingSnapshot;
import com.springboot.model.PortfolioSnapshot;
import com.springboot.service.HoldingStreamService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.format.annotation.DateTimeFormat;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;

import java.time.LocalDateTime;
import java.util.Collection;
import java.util.List;

@CrossOrigin(origins = "*")
@RestController
@RequestMapping("/stream")
public class HoldingController
{
    private final HoldingStreamService service;

    @Autowired
    public HoldingController(HoldingStreamService service)
    {
        this.service = service;
    }

    @GetMapping(value = "/holdings", produces = MediaType.TEXT_EVENT_STREAM_VALUE)
    public SseEmitter StreamHoldings()
    {
        // 각 클라이언트마다 새로운 SseEmitter를 생성하여 반환
        return service.CreateEmitter();
    }

    @GetMapping("/holdings/latest")
    public Collection<Holding> GetLastest()
    {
        // 최신 보유 정보를 반환하는 엔드포인트 (메모리에서)
        return service.GetLatestHoldings();
    }
    
    // 데이터베이스에서 최신 보유 정보 조회
    @GetMapping("/holdings/latest/db")
    public List<HoldingSnapshot> GetLatestFromDB()
    {
        return service.GetLatestHoldingsFromDB();
    }
    
    // 포트폴리오 이력 조회 (특정 기간)
    @GetMapping("/portfolio/history")
    public List<PortfolioSnapshot> GetPortfolioHistory(
            @RequestParam @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) LocalDateTime startTime,
            @RequestParam(required = false) @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) LocalDateTime endTime)
    {
        return service.GetPortfolioHistory(startTime, endTime);
    }
    
    // 포트폴리오 최근 N개 스냅샷 조회
    @GetMapping("/portfolio/recent")
    public List<PortfolioSnapshot> GetRecentPortfolioSnapshots(
            @RequestParam(defaultValue = "100") int limit)
    {
        // 최대 1000개로 제한
        if (limit > 1000)
            limit = 1000;

        return service.GetRecentPortfolioSnapshots(limit);
    }
    
    // 포트폴리오 최근 24시간 데이터 조회 (편의 메서드)
    @GetMapping("/portfolio/recent24h")
    public List<PortfolioSnapshot> GetRecent24Hours()
    {
        LocalDateTime startTime = LocalDateTime.now().minusHours(24);

        return service.GetPortfolioHistory(startTime, null);
    }
    
    // 포트폴리오 최근 7일 데이터 조회 (편의 메서드)
    @GetMapping("/portfolio/recent7d")
    public List<PortfolioSnapshot> GetRecent7Days()
    {
        LocalDateTime startTime = LocalDateTime.now().minusDays(7);
        
        return service.GetPortfolioHistory(startTime, null);
    }
}
