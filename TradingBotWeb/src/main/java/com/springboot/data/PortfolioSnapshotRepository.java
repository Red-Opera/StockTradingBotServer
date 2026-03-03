package com.springboot.data;

import com.springboot.model.PortfolioSnapshot;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.stereotype.Repository;

import java.time.LocalDateTime;
import java.util.List;
import java.util.Optional;

@Repository
public interface PortfolioSnapshotRepository extends JpaRepository<PortfolioSnapshot, Long>
{
    // 특정 시간 이후의 스냅샷 조회 (시간 순 정렬)
    List<PortfolioSnapshot> findBySnapshotTimeAfterOrderBySnapshotTimeAsc(LocalDateTime startTime);
    
    // 특정 시간 범위의 스냅샷 조회
    List<PortfolioSnapshot> findBySnapshotTimeBetweenOrderBySnapshotTimeAsc(LocalDateTime startTime, LocalDateTime endTime);
    
    // 최근 N개의 스냅샷 조회
    @Query("SELECT p FROM PortfolioSnapshot p ORDER BY p.snapshotTime DESC LIMIT :limit")
    List<PortfolioSnapshot> findTopNByOrderBySnapshotTimeDesc(@Param("limit") int limit);
    
    // 가장 최근 스냅샷 조회
    Optional<PortfolioSnapshot> findTopByOrderBySnapshotTimeDesc();
    
    // 특정 날짜의 스냅샷 개수
    @Query("SELECT COUNT(p) FROM PortfolioSnapshot p WHERE DATE(p.snapshotTime) = DATE(:date)")
    long countByDate(@Param("date") LocalDateTime date);
}
