package com.springboot.controller;

import com.springboot.model.Holding;
import com.springboot.service.HoldingStreamService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.CrossOrigin;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;

import java.util.Collection;

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
        // 최신 보유 정보를 반환하는 엔드포인트
        return service.GetLatestHoldings();
    }
}
