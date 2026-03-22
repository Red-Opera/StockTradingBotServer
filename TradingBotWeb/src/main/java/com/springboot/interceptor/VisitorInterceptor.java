package com.springboot.interceptor;

import com.springboot.data.VisitorService;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import jakarta.xml.bind.DatatypeConverter;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;
import org.springframework.web.servlet.HandlerInterceptor;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

@Component
public class VisitorInterceptor implements HandlerInterceptor {

    private final VisitorService visitorService;

    @Autowired
    public VisitorInterceptor(VisitorService visitorService) {
        this.visitorService = visitorService;
    }

    @Override
    public boolean preHandle(HttpServletRequest request, HttpServletResponse response, Object handler) throws Exception {
        try {
            String ipAddress = request.getRemoteAddr();
            String userAgent = request.getHeader("User-Agent");
            if (userAgent == null) userAgent = "";

            // 식별자 생성 및 증가 시도
            String visitorIdentifier = generateVisitorIdentifier(ipAddress, userAgent);
            visitorService.incrementDailyVisitors(visitorIdentifier);
        } 
        
        catch (Exception ex)
        {
            // 방문자 집계는 보조 기능이므로 실패해도 요청 처리에 영향 주지 않음
        }

        return true;
    }

    private String generateVisitorIdentifier(String ip, String userAgent)
    {
        try
        {
            MessageDigest md = MessageDigest.getInstance("MD5");
            md.update((ip + userAgent).getBytes());
            byte[] digest = md.digest();

            return DatatypeConverter.printHexBinary(digest).toUpperCase();
        }
        
        catch (NoSuchAlgorithmException e)
        {
            return ip + userAgent;
        }
    }
}
