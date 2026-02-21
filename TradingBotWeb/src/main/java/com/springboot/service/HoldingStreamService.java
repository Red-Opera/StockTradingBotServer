package com.springboot.service;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.springboot.model.Holding;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;

import jakarta.annotation.PreDestroy;
import jakarta.annotation.PostConstruct;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

@Service
public class HoldingStreamService
{
    private static final Logger log = LoggerFactory.getLogger(HoldingStreamService.class);
    private volatile boolean running = true;

    private final List<SseEmitter> emitters = new ArrayList<>();                    // 현재 연결된 모든 SseEmitter를 저장하는 리스트
    private final Map<String, Holding> latest = new ConcurrentHashMap<>();          // 종목 코드별 최신 Holding 정보를 저장하는 맵
    private final ObjectMapper mapper = new ObjectMapper();                         // JSON 문자열을 Holding 객체로 변환하기 위한 ObjectMapper
    private final ExecutorService executor = Executors.newSingleThreadExecutor();   // 소켓 연결과 데이터 수신을 처리할 단일 스레드 ExecutorService

    @PostConstruct
    public void Start() 
    {
        executor.submit(this::ConnectLoop);
    }

    @PreDestroy
    public void Stop()
    {
        running = false;

        executor.shutdownNow();
    }

    // 클라이언트가 SSE 스트림을 구독할 때마다 새로운 SseEmitter를 생성하여 반환하는 메서드
    public SseEmitter CreateEmitter()
    {
        SseEmitter emitter = new SseEmitter(Long.MAX_VALUE);

        synchronized (emitters) { emitters.add(emitter); }

        emitter.onCompletion(() -> RemoveEmitter(emitter));
        emitter.onTimeout(() -> RemoveEmitter(emitter));

        return emitter;
    }

    // SseEmitter를 리스트에서 제거하는 메서드
    private void RemoveEmitter(SseEmitter emitter)
    {
        synchronized (emitters) { emitters.remove(emitter); }
    }

    // 최신 보유 정보를 반환하는 메서드
    public Collection<Holding> GetLatestHoldings()
    {
        return latest.values();
    }

    // 새로운 Holding 정보가 수신될 때마다 모든 연결된 클라이언트에게 해당 정보를 전송하는 메서드
    private void Broadcast(Holding holding)
    {
        synchronized (emitters)
        {
            List<SseEmitter> toRemove = new ArrayList<>();

            for (SseEmitter emitter : emitters)
            {
                try
                {
                    emitter.send(SseEmitter.event().name("holding").data(holding));
                }
                
                catch (Exception ex)
                {
                    toRemove.add(emitter);
                }
            }

            emitters.removeAll(toRemove);
        }
    }

    private void ConnectLoop()
    {
        while (running)
        {
            try (
                    Socket socket = new Socket("localhost", 9000);
                    BufferedReader reader = new BufferedReader(new InputStreamReader(socket.getInputStream(), StandardCharsets.UTF_8))
                )
            {
                log.info("Connected to localhost:9000 for holdings stream");
                String line;

                while (running && (line = reader.readLine()) != null)
                {
                    line = line.trim();
                    
                    if (line.isEmpty())
                        continue;

                    try
                    {
                        Holding h = mapper.readValue(line, Holding.class);
                        latest.put(h.getCode(), h);

                        try
                        {
                            log.info("Received holding: {}", mapper.writeValueAsString(h));
                        }

                        catch (Exception ignore)
                        {

                        }

                        Broadcast(h);
                    }
                    
                    catch (Exception e)
                    {
                        log.warn("Failed to parse holding json: {}", line, e);
                    }
                }
            }
            
            catch (Exception e)
            {
                log.warn("Socket read error or connection failed, will retry in 2s", e);

                try
                {
                    Thread.sleep(2000);
                }
                
                catch (InterruptedException ie)
                {
                    Thread.currentThread().interrupt();
                    break;
                }
            }
        }
    }
}
