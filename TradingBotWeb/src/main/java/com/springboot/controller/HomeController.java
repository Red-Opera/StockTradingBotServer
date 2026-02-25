package com.springboot.controller;

import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.GetMapping;
import com.springboot.model.VisitorCount;

@Controller
public class HomeController 
{
    @GetMapping("/")
    public String home(Model model) 
    {
        return "home";
    }

    @GetMapping("/support")
    public String support(Model model) 
    {
        model.addAttribute("message", "Support");
        model.addAttribute("visitorCount", new VisitorCount(0, 0)); // 임시로 0으로 설정
        
        return "support";
    }
}