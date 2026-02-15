package com.springboot.controller;

import com.springboot.model.Holding;
import com.springboot.service.HoldingStreamService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;

import java.util.Collection;

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
        return service.CreateEmitter();
    }

    @GetMapping("/holdings/latest")
    public Collection<Holding> GetLastest()
    {
        return service.GetLatestHoldings();
    }
}
